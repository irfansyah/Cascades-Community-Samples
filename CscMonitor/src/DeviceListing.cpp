/* Copyright (c) 2013 Research In Motion Limited.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "DeviceListing.hpp"
#include "Utilities.hpp"

DeviceListing::DeviceListing(QObject *parent) :
		QObject(parent), _model(new bb::cascades::GroupDataModel(QStringList() << "deviceName" << "deviceAddress" << "deviceClass" << "deviceType", this)), CSC_SERVICE_UUID("1816") {
	_model->setSortingKeys(QStringList() << "deviceType");
	_model->setGrouping(bb::cascades::ItemGrouping::ByFullValue);

	if (!Utilities::getOSVersion().startsWith("10.0")) {
		CSC_SERVICE_UUID.prepend("0x");
	}
}

void DeviceListing::update() {
	qDebug() << "YYYY DeviceListing::update() - clearing model";

	_model->clear();

	bt_remote_device_t **remoteDeviceArray;
	bt_remote_device_t *nextRemoteDevice = 0;

	qDebug() << "YYYY DeviceListing::update() - about to call bt_disc_retrieve_devices(BT_DISCOVERY_ALL, 0)";

	remoteDeviceArray = bt_disc_retrieve_devices(BT_DISCOVERY_ALL, 0);

	qDebug() << "YYYY DeviceListing::update() - returned from bt_disc_retrieve_devices(BT_DISCOVERY_ALL, 0)";

	if (remoteDeviceArray) {
		qDebug() << "YYYY DeviceListing::update() - found devices";
		for (int i = 0; (nextRemoteDevice = remoteDeviceArray[i]); ++i) {
			qDebug() << "DEVICE ----------------------------------------------------------------------------------------";
			QVariantMap map;
			char buffer[128];
			const int bufferSize = sizeof(buffer);

			qDebug() << "YYYY DeviceListing::update() - checking device";

			if (isACscMonitor(nextRemoteDevice)) {
				qDebug() << "YYYY DeviceListing::update() - found a CSC device";
				bt_rdev_get_friendly_name(nextRemoteDevice, buffer, bufferSize);
				map["deviceName"] = QString::fromLatin1(buffer);
				qDebug() << "YYYY DeviceListing::update() - name=" << QString::fromLatin1(buffer);
				bt_rdev_get_address(nextRemoteDevice, buffer);
				map["deviceAddress"] = QString::fromLatin1(buffer);
				map["deviceClass"] = QString::number(bt_rdev_get_device_class(nextRemoteDevice, BT_COD_DEVICECLASS));
				map["deviceType"] = tr("Paired Bluetooth Devices");
				qDebug() << "YYYY DeviceListing::update() - address=" << QString::fromLatin1(buffer);
				_model->insert(map);
			} else {
				qDebug() << "YYYY DeviceListing::update() - not an CSC device";
			}
		}
		qDebug() << "YYYY DeviceListing::update() - freeing buffer";
		bt_rdev_free_array(remoteDeviceArray);
	}
	qDebug() << "YYYY DeviceListing::update() - returning";
}

void DeviceListing::discover() {
	qDebug() << "YYYY DeviceListing::discover()";
	bb::system::SystemToast toast;
	toast.setBody(tr("Searching for Bluetooth CSC Monitors ... please wait until search has completed ..."));
	toast.setPosition(bb::system::SystemUiPosition::MiddleCenter);
	toast.exec();

	bt_disc_start_inquiry(BT_INQUIRY_GIAC);
	delay(5);
	bt_disc_cancel_inquiry();

	toast.setBody(tr("Search completed!"));
	toast.exec();

	update();

	bt_remote_device_t *nextRemoteDevice = 0;

	qDebug() << "YYYY DeviceListing::discover() - about to call bt_disc_retrieve_devices(BT_DISCOVERY_ALL, 0)";

	bt_remote_device_t **remoteDeviceArray = bt_disc_retrieve_devices(BT_DISCOVERY_ALL, 0);

	qDebug() << "YYYY DeviceListing::discover() - returned from bt_disc_retrieve_devices(BT_DISCOVERY_ALL, 0)";

	if (remoteDeviceArray) {
		qDebug() << "YYYY DeviceListing::discover() - found devices";

		for (int i = 0; (nextRemoteDevice = remoteDeviceArray[i]); ++i) {
			QVariantMap map;
			char buffer[128];
			const int bufferSize = sizeof(buffer);

			qDebug() << "YYYY DeviceListing::discover() - checking device";

			if (isACscMonitor(nextRemoteDevice)) {
				qDebug() << "YYYY DeviceListing::discover() - found an CSC device";
				bt_rdev_get_friendly_name(nextRemoteDevice, buffer, bufferSize);
				map["deviceName"] = QString::fromLatin1(buffer);
				qDebug() << "YYYY DeviceListing::discover() - name=" << QString::fromLatin1(buffer);
				bt_rdev_get_address(nextRemoteDevice, buffer);
				map["deviceAddress"] = QString::fromLatin1(buffer);
				map["deviceClass"] = QString::number(bt_rdev_get_device_class(nextRemoteDevice, BT_COD_DEVICECLASS));
				map["deviceType"] = tr("Bluetooth Devices Nearby");
				qDebug() << "YYYY DeviceListing::discover() - address=" << QString::fromLatin1(buffer);
				_model->insert(map);
			} else {
				qDebug() << "YYYY DeviceListing::discover() - not an CSC device";
			}
		}
		qDebug() << "YYYY DeviceListing::discover() - freeing buffer";

		bt_rdev_free_array(remoteDeviceArray);
	}
	qDebug() << "YYYY DeviceListing::discover() - returning";
}

bool DeviceListing::isACscMonitor(bt_remote_device_t * remoteDevice) {

	qDebug() << "YYYY DeviceListing::isACscMonitor() - entering";

	bool foundMonitor = false;

	if (!remoteDevice) {
		qDebug() << "YYYY DeviceListing::isACscMonitor() - no devices - exiting";
		return foundMonitor;
	}
	qDebug() << "YYYY DeviceListing::isACscMonitor() - checking device type";

	const int deviceType = bt_rdev_get_type(remoteDevice);
	qDebug() << "YYYY DeviceListing::isACscMonitor() - deviceType=" << deviceType;
	char **servicesArray = bt_rdev_get_services_gatt(remoteDevice);
	qDebug() << "YYYY DeviceListing::isACscMonitor() - checking services";
	if (servicesArray) {
		qDebug() << "YYYY DeviceListing::isACscMonitor() - found services";
		for (int i = 0; servicesArray[i]; i++) {
			qDebug() << "YYYY DeviceListing::isACscMonitor() - Checking type = " << servicesArray[i];
			if (strcasecmp(servicesArray[i], CSC_SERVICE_UUID.toAscii().constData()) == 0) {
				qDebug() << "YYYY DeviceListing::isACscMonitor() - CSC service found";
				foundMonitor = true;
				break;
			} else {
				qDebug() << "YYYY DeviceListing::isACscMonitor() - not an CSC service";
			}
		}
		qDebug() << "YYYY DeviceListing::isACscMonitor() - freeing buffer";
		bt_rdev_free_services(servicesArray);
	} else {
		qDebug() << "YYYY DeviceListing::isACscMonitor() - no services found";

	}
	qDebug() << "YYYY DeviceListing::isACscMonitor() - returning " << foundMonitor;

	return foundMonitor;
}

bb::cascades::DataModel* DeviceListing::model() const {
	return _model;
}