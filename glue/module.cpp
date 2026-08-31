#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <vector>
extern "C" {
#include <runtime.h>
#include <runtime_ext.h>
#include <wifi.h>
#include <bluetooth.h>
};

#include "NimBLEDevice.h"
#include "NimBLEClient.h"
#include "NimBLEAdvertisedDevice.h"

#include "Device.h"
#include "CameraList.h"
#include "CanonEOSRemote.h"
#include "CanonEOSSmart.h"
#include "FauxNY.h"
#include "FujifilmBasic.h"
#include "FujifilmSecure.h"
#include "Nikon.h"
#include "Ricoh.h"
#include "Sony.h"

using namespace Furble;

void nimble_set_pak_device(struct PakBt *ctx, struct PakBtDevice *dev);

/*
 * The granted device is always matched fresh as PairType::NEW, so no
 * NimBLE scan path (FujifilmSecure/Nikon saved-reconnect) is ever reached.
 * Keep it that way: Scan::getInstance() aborts.
 */
struct ModulePriv {
	Camera *camera;
	struct PakBtDevice *dev;
};

/*
 * Saved-connection aux blob: [Camera::Type u32 LE][serialised nvs_t].
 * The type tag selects the camera class so the blob can be reconstructed
 * (mirrors CameraList::load); the serialised nvs_t carries name, MAC, type
 * and the pairing token/serial.
 */
typedef struct __attribute__((packed)) {
	uint32_t type;
} saved_hdr_t;

static Camera *camera_from_blob(Camera::Type type, const void *data, size_t len) {
	switch (type) {
	case Camera::Type::FUJIFILM_BASIC:
		return new FujifilmBasic(data, len);
	case Camera::Type::FUJIFILM_SECURE:
		return new FujifilmSecure(data, len);
	case Camera::Type::CANON_EOS_SMART:
		return new CanonEOSSmart(data, len);
	case Camera::Type::CANON_EOS_REMOTE:
		return new CanonEOSRemote(data, len);
	case Camera::Type::FAUXNY:
		return new FauxNY(data, len);
	case Camera::Type::NIKON:
		return new Nikon(data, len);
	case Camera::Type::SONY:
		return new Sony(data, len);
	case Camera::Type::RICOH:
		return new Ricoh(data, len);
	default:
		return nullptr;
	}
}

static int init(struct PakModule *mod) {
	pak_debug_log(mod, "Hello from furble module");
	Device::init(ESP_PWR_LVL_P3);
	mod->priv = static_cast<struct ModulePriv *>(calloc(sizeof(struct ModulePriv), 1));
	pak_rt_set_screen_supported(mod, PAK_SCREEN_DASHBOARD, 1);
	pak_rt_set_screen_supported(mod, PAK_SCREEN_INTERVALOMETER, 1);
	pak_rt_set_tick_interval(mod, 1000 * 100);

	/* The intervalometer screen is built into the host (shot timer +
	 * shutter/focus controls) and drives capture through PAK_CMD_* custom
	 * commands - no module-side widgets needed. */

	return 0;
}

static void drop_camera(struct PakModule *mod) {
	struct ModulePriv *priv = static_cast<struct ModulePriv *>(mod->priv);
	if (priv->camera) {
		priv->camera->disconnect();
		priv->camera = nullptr;
	}
	nimble_set_pak_device(nullptr, nullptr);
	CameraList::clear();
	if (priv->dev) {
		pak_bt_unref_device(mod->bt, priv->dev);
		priv->dev = nullptr;
	}
}

static int on_free(struct PakModule *mod) {
	drop_camera(mod);
	free(mod->priv);
	mod->priv = nullptr;
	return 0;
}

static int on_try_connect_bluetoth(struct PakModule *mod, struct PakBtDevice *dev, struct PakSavedConnection *saved, int job) {
	(void)job;
	if (!dev) return PAK_ERR_NO_CONNECTION;

	struct ModulePriv *priv = static_cast<struct ModulePriv *>(mod->priv);
	nimble_set_pak_device(mod->bt, dev);
	NimBLEAdvertisedDevice nimbleDevice = NimBLEAdvertisedDevice(mod->bt, dev);

	pak_debug_log(mod, "granted: mfr=%d mfrLen=%u svc0=%s",
		      (int)nimbleDevice.haveManufacturerData(),
		      (unsigned)nimbleDevice.getManufacturerData().length(),
		      nimbleDevice.haveServiceUUID() ? nimbleDevice.getServiceUUID().toString().c_str() : "none");

	CameraList::clear();
	if (saved && saved->aux_data && saved->aux_data_length > sizeof(saved_hdr_t)) {
		/* Saved reconnect: the aux blob carries the camera type and its
		 * serialised state (pairing token/serial), so no advertisement
		 * match is needed. */
		saved_hdr_t hdr;
		memcpy(&hdr, saved->aux_data, sizeof(hdr));
		pak_debug_log(mod, "Saved connection type=%u", hdr.type);
		priv->camera =
			camera_from_blob(static_cast<Camera::Type>(hdr.type),
					 saved->aux_data + sizeof(hdr),
					 saved->aux_data_length - sizeof(hdr));
		if (!priv->camera) {
			pak_debug_log(mod, "Saved blob has unknown camera type");
			return PAK_ERR_NO_CONNECTION;
		}
	} else if (!CameraList::match(&nimbleDevice) || CameraList::size() == 0) {
		pak_debug_log(mod, "Granted device matches no camera type");
		return PAK_ERR_NO_CONNECTION;
	} else {
		priv->camera = CameraList::last();
	}
	priv->dev = dev;

	if (!priv->camera->connect(ESP_PWR_LVL_P3, 10000)) {
		pak_debug_log(mod, "Camera connect failed");
		drop_camera(mod);
		return PAK_ERR_NO_CONNECTION;
	}
	nimble_set_pak_device(nullptr, nullptr);

	pak_rt_set_session_property(mod, PAK_PROP_NAME, priv->camera->getName().c_str());

	/* Persist the pairing data so the next session can skip pairing
	 * entirely: [type][nvs_t] as aux, camera name as identity. */
	size_t dbytes = priv->camera->getSerialisedBytes();
	std::vector<uint8_t> aux(sizeof(saved_hdr_t) + dbytes);
	saved_hdr_t hdr;
	hdr.type = static_cast<uint32_t>(priv->camera->getType());
	memcpy(aux.data(), &hdr, sizeof(hdr));
	if (priv->camera->serialise(aux.data() + sizeof(hdr), dbytes)) {
		struct PakSavedConnection out = {};
		const std::string &name = priv->camera->getName();
		out.unique_id = name.c_str();
		out.name = name.c_str();
		out.aux_data = aux.data();
		out.aux_data_length = aux.size();
		if (pak_rt_save_session_signature(mod, &out) != 0) {
			pak_debug_log(mod, "Saving session signature failed");
		} else {
			pak_debug_log(mod, "Saved connection (%u aux bytes)",
				      (unsigned)aux.size());
		}
	} else {
		pak_debug_log(mod, "Camera serialisation failed");
	}

	return 0;
}

static int on_run_test(struct PakModule *mod, int job) {
	(void)mod;
	(void)job;
	pak_global_log("Hello");
	return 0;
}

static int on_idle_tick(struct PakModule *mod, unsigned int us_since_last_tick) {
	(void)us_since_last_tick;
	struct ModulePriv *priv = static_cast<struct ModulePriv *>(mod->priv);
	if (!priv || !priv->dev) return 0;
	pak_bt_device_update(mod->bt, priv->dev);
	if (!priv->dev->is_connected) {
		pak_rt_fatal_error(mod, "Camera disconnected");
	}
	return 0;
}

static int on_disconnect(struct PakModule *mod) {
	drop_camera(mod);
	return 0;
}

static int on_switch_screen(struct PakModule *mod, int old_screen, int new_screen, int job) {
	(void)mod;
	(void)old_screen;
	(void)new_screen;
	(void)job;
	return 0;
}

static int on_custom_command(struct PakModule *mod, int job, int argc, const char * const *argv) {
	(void)job;
	struct ModulePriv *priv = static_cast<struct ModulePriv *>(mod->priv);
	if (!priv->camera || argc < 1) return 0;
	const char *cmd = argv[0];
	if (!strcmp(cmd, PAK_CMD_SHUTTER_DOWN)) priv->camera->shutterPress();
	else if (!strcmp(cmd, PAK_CMD_SHUTTER_UP)) priv->camera->shutterRelease();
	else if (!strcmp(cmd, PAK_CMD_FOCUS_DOWN)) priv->camera->focusPress();
	else if (!strcmp(cmd, PAK_CMD_FOCUS_UP)) priv->camera->focusRelease();
	return 0;
}

static int on_prop_changed(struct PakModule *mod, int job, const char *name, struct PakWidget *prop) {
	(void)job;
	(void)prop;
	struct ModulePriv *priv = static_cast<struct ModulePriv *>(mod->priv);
	if (!priv->camera) return 0;
	if (!strcmp(name, "shutter")) {
		priv->camera->shutterPress();
		priv->camera->shutterRelease();
	} else if (!strcmp(name, "focus")) {
		priv->camera->focusPress();
		priv->camera->focusRelease();
	}
	return 0;
}

extern "C" int get_module(struct PakModule *mod) {
	mod->init = init;
	mod->free = on_free;
	mod->on_try_connect_bluetooth = on_try_connect_bluetoth;
	mod->on_idle_tick = on_idle_tick;
	mod->on_disconnect = on_disconnect;
	mod->on_switch_screen = on_switch_screen;
	mod->on_custom_command = on_custom_command;
	mod->on_setting_changed = on_prop_changed;
	mod->on_run_test = on_run_test;
	return 0;
}
