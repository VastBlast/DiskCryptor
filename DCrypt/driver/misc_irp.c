/*
    *
    * DiskCryptor - open source partition encryption tool
	* Copyright (c) 2026
	* DavidXanatos <info@diskcryptor.org>
    * Copyright (c) 2007-2013
    * ntldr <ntldr@diskcryptor.net> PGP key ID - 0xC48251EB4F8E4E6E
    *

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3 as
    published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <ntifs.h>
#include "defines.h"
#include "devhook.h"
#include "misc_irp.h"
#include "misc.h"
#include "driver.h"
#include "mount.h"
#include "enc_dec.h"
#include "debug.h"
#include "dump_hook.h"
#include "misc_mem.h"
#include "boot_pass.h"
#include "crypto_functions.h"

typedef struct _pw_irp_ctx {
	WORK_QUEUE_ITEM  wrk_item;
	dev_hook        *hook;
	PIRP             irp;

} pw_irp_ctx;

/* function types declaration */
IO_COMPLETION_ROUTINE dc_sync_complete;
WORKER_THREAD_ROUTINE dc_power_irp_worker;

NTSTATUS dc_complete_irp(PIRP irp, NTSTATUS status, ULONG_PTR bytes)
{
	irp->IoStatus.Status      = status;
	irp->IoStatus.Information = bytes;

	IoCompleteRequest(irp, IO_NO_INCREMENT);

	return status;
}

NTSTATUS dc_release_irp(dev_hook *hook, PIRP irp, NTSTATUS status)
{
	dc_complete_irp(irp, status, 0);
	IoReleaseRemoveLock(&hook->remove_lock, irp);
	return status;
}

NTSTATUS dc_forward_irp(dev_hook *hook, PIRP irp)
{
	NTSTATUS status;

	IoSkipCurrentIrpStackLocation(irp);
	status = IoCallDriver(hook->orig_dev, irp);
			
	IoReleaseRemoveLock(&hook->remove_lock, irp);
	return status;
}

static NTSTATUS dc_sync_complete(PDEVICE_OBJECT dev_obj, PIRP irp, PKEVENT sync)
{
	KeSetEvent(sync, IO_NO_INCREMENT, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS dc_forward_irp_sync(dev_hook *hook, PIRP irp)
{
	KEVENT   sync;
	NTSTATUS status;

	KeInitializeEvent(&sync, NotificationEvent, FALSE);
	IoCopyCurrentIrpStackLocationToNext(irp);
    IoSetCompletionRoutine(irp, dc_sync_complete, &sync, TRUE, TRUE, TRUE);

	status = IoCallDriver(hook->orig_dev, irp);

    if (status == STATUS_PENDING) {
		wait_object_infinity(&sync);
		status = irp->IoStatus.Status;
    }
	return status;
}


NTSTATUS dc_create_close_irp(PDEVICE_OBJECT dev_obj, PIRP irp)
{
	PIO_STACK_LOCATION irp_sp = IoGetCurrentIrpStackLocation(irp);
	NTSTATUS           status = STATUS_SUCCESS;
	PEPROCESS          process;
	PACCESS_TOKEN      token;

	/* get requestor process */
	process = IoGetRequestorProcess(irp);
	if (process == NULL) process = IoGetCurrentProcess();
	
	if (irp_sp->MajorFunction == IRP_MJ_CREATE)
	{
		/* check process token */
		if ( (token = PsReferencePrimaryToken(process)) == NULL || SeTokenIsAdmin(token) == FALSE ) {
			status = STATUS_ACCESS_DENIED;
		}
		if (token != NULL) PsDereferencePrimaryToken(token);
	}
	if (irp_sp->MajorFunction == IRP_MJ_CLOSE) {
		// synchronize all encryptions
		dc_sync_all_encs();
	}
	return dc_complete_irp(irp, status, 0);
}

/*
* Erase encryption keys for all F_NO_HIBER volumes before hibernation.
* This prevents key material from being written to the hibernation file.                                                                    
*/
static void dc_prepare_hiber_erase_unmount(dev_hook *hook)
{
	DbgMsg("erasing keys for hibernation, dev=%ws\n", hook->dev_name);

	// Block I/O and mark for post-resume cleanup
	hook->flags |= (F_DISABLE | F_HIBER_ERASE);

	// Securely erase all encryption keys
	if (hook->dsk_key != NULL) {
		RtlSecureZeroMemory(hook->dsk_key, sizeof(xts_key));
	}
	if (hook->tmp_key != NULL) {
		RtlSecureZeroMemory(hook->tmp_key, sizeof(xts_key));
	}
	if (hook->hdr_key != NULL) {
		RtlSecureZeroMemory(hook->hdr_key, sizeof(xts_key));
	}
	if (hook->bak_key != NULL) {
		RtlSecureZeroMemory(hook->bak_key, sizeof(xts_key));
	}

	// Flush CPU caches to ensure keys not recoverable
	__wbinvd();
}

/*
 * Complete unmount and attempt auto-remount for a single F_HIBER_ERASE volume.
 * Called after resume from hibernation with busy_lock NOT held.
 */
static void dc_complete_hiber_erase_unmount(dev_hook *hook)
{
	DbgMsg("attempting auto-remount after resume, dev=%ws\n", hook->dev_name);

	wait_object_infinity(&hook->busy_lock);

	// Complete the unmount (free zeroed key structures)
	hook->flags &= ~(F_CLEAR_ON_UNMOUNT | F_DISABLE);
	hook->flags |= F_FS_RAW;

	if (hook->dsk_key != NULL) {
		mm_secure_free(hook->dsk_key);
		hook->dsk_key = NULL;
	}
	if (hook->tmp_key != NULL) {
		mm_secure_free(hook->tmp_key);
		hook->tmp_key = NULL;
	}
	if (hook->hdr_key != NULL) {
		mm_secure_free(hook->hdr_key);
		hook->hdr_key = NULL;
	}
	if (hook->bak_key != NULL) {
		mm_secure_free(hook->bak_key);
		hook->bak_key = NULL;
	}

	hook->use_size = hook->dsk_size;
	hook->tmp_size = 0;
	lock_inc(&hook->chg_mount);

	KeReleaseMutex(&hook->busy_lock, FALSE);

	// Attempt auto-remount using cached passwords/boot keys
	// dc_mount_device acquires busy_lock internally, so we must release it first
	if (dc_mount_device(hook->dev_name, NULL, MF_NO_HIBER, NULL) == ST_OK)
	{
		DbgMsg("auto-remount successful, dev=%ws\n", hook->dev_name);
	}
	else
	{
		DbgMsg("auto-remount failed (no cached password), dev=%ws\n", hook->dev_name);
	}
}

/*
 * Complete unmount for all F_HIBER_ERASE volumes after resume from hibernation.
 * Must be called with NO busy_lock held.
 */
static void dc_complete_pending_unmounts()
{
	dev_hook *hook;

	for (hook = dc_first_hook(); hook != NULL; hook = dc_next_hook(hook))
	{
		if (hook->flags & F_HIBER_ERASE)
		{
			dc_complete_hiber_erase_unmount(hook);
		}
	}
}

static
NTSTATUS dc_process_power_irp(dev_hook *hook, PIRP irp)
{
	NTSTATUS           status;
	PIO_STACK_LOCATION irp_sp;
	int                do_critical_unmounts = 0;

	irp_sp = IoGetCurrentIrpStackLocation(irp);

	if ( irp_sp->MinorFunction == IRP_MN_SET_POWER && irp_sp->Parameters.Power.Type == SystemPowerState )
	{
		wait_object_infinity(&hook->busy_lock);

		if (irp_sp->Parameters.Power.State.SystemState == PowerSystemHibernate)
		{
			// prevent device encryption to sync device and memory state
			hook->flags |= F_PREVENT_ENC;
#ifdef DC_CONCURRENT_TRANSCRYPT
			if (hook->flags & F_SYNC) {
				/* Wait for any in-progress transcryption to complete.
				 * dc_transcrypt_step checks F_PREVENT_ENC and will exit early,
				 * but we need to wait for any step that's already past that check. */
				dc_wait_for_transcrypt(hook);
				/* Save current progress */
				dc_save_enc_state(hook, SYNC_STEP_UPDATE);
			}
#else
			dc_send_sync_packet(hook->dev_name, S_OP_SYNC, 0);
#endif

			// Erase keys for THIS F_NO_HIBER volume BEFORE hibernation
			// Each device receives its own power IRP, so we only handle this hook
			if ((hook->flags & F_ENABLED) && (hook->flags & F_NO_HIBER))
			{
				dc_prepare_hiber_erase_unmount(hook);
			}
		}

		if (irp_sp->Parameters.Power.State.SystemState == PowerSystemWorking)
		{
			if (hook->pdo_dev->Flags & DO_SYSTEM_BOOT_PARTITION)
			{
				// search bootloader password in memory after hibernation
				dc_get_boot_pass();

				// initialize encryption again, because CPU capabilities may be changed
				dc_init_encryption();

				// Mark that we need to do critical unmounts after releasing busy_lock
				do_critical_unmounts = 1;
			}
			// allow encryption requests
			hook->flags &= ~F_PREVENT_ENC;
		}

		KeReleaseMutex(&hook->busy_lock, FALSE);

		// Complete unmount and attempt auto-remount for F_NO_HIBER volumes
		// This must be done AFTER releasing busy_lock to avoid deadlock with dc_mount_device
		if (do_critical_unmounts)
		{
			dc_complete_pending_unmounts();

			// clean password cache if CONF_CACHE_PASSWORD is not set
			dc_clear_secrets(FALSE);
		}
	}

	PoStartNextPowerIrp(irp);
	IoSkipCurrentIrpStackLocation(irp);
	status = PoCallDriver(hook->orig_dev, irp);

	IoReleaseRemoveLock(&hook->remove_lock, irp);
	return status;
}

static void dc_power_irp_worker(pw_irp_ctx *pwc)
{
	dc_process_power_irp(pwc->hook, pwc->irp);
	mm_pool_free(pwc);
}

NTSTATUS dc_power_irp(dev_hook *hook, PIRP irp)
{
	pw_irp_ctx *pwc;

	if (KeGetCurrentIrql() == PASSIVE_LEVEL) {
		return dc_process_power_irp(hook, irp);
	}

	if ( (pwc = mm_pool_alloc(sizeof(pw_irp_ctx))) == NULL )
	{
		PoStartNextPowerIrp(irp);		
		return dc_release_irp(hook, irp, STATUS_INSUFFICIENT_RESOURCES);
	}

	pwc->hook = hook;
	pwc->irp  = irp;

	IoMarkIrpPending(irp);

	ExInitializeWorkItem(&pwc->wrk_item, dc_power_irp_worker, pwc);
	ExQueueWorkItem(&pwc->wrk_item, DelayedWorkQueue);

	return STATUS_PENDING;
}
