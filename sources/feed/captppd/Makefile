include $(TOPDIR)/rules.mk

PKG_NAME:=captppd
PKG_VERSION:=1.0.0
PKG_RELEASE:=1

# Vendored sources are pinned in the accompanying build bundle.
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=file://$(TOPDIR)/dl
PKG_HASH:=9ae649002c46def370a043885121bc15aece8527f0bdaab3f8b5f54f9d703c8e

PKG_LICENSE:=BSD-2-Clause
PKG_LICENSE_FILES:=LICENSE libcapt-src/LICENSE
PKG_BUILD_PARALLEL:=1
PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/captppd
  SECTION:=net
  CATEGORY:=Network
  SUBMENU:=Printing
  TITLE:=Canon CAPT v1 backend for LBP-810
  DEPENDS:=+cups +libcups +libusb-1.0 +libstdcpp
endef

define Package/captppd/description
  Open source libusb CUPS backend for Canon LBP-810 CAPT v1 printers,
  based on captppd and libcapt by Andrey Antufyev.
endef

CMAKE_OPTIONS += \
	-DCAPTPPD_EMBEDDED=ON \
	-DCAPTPPD_BUILD_TESTS=OFF \
	-DCUPS_CONFIG=/bin/true \
	-DCUPS_SERVER_BIN=/usr/lib/cups \
	-DCUPS_DATA_DIR=/usr/share/cups \
	-DCUPS_CFLAGS= \
	-DCUPS_LDFLAGS= \
	-DCUPS_LIBS=-lcups

define Package/captppd/install
	$(INSTALL_DIR) $(1)/usr/lib/cups/backend
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/lib/cups/backend/captusb $(1)/usr/lib/cups/backend/
	chmod 0700 $(1)/usr/lib/cups/backend/captusb
	$(INSTALL_DIR) $(1)/usr/share/cups/model
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/ppd/LBP810CAPTPPD.ppd $(1)/usr/share/cups/model/
	$(INSTALL_DIR) $(1)/usr/share/cups/usb
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/dist/capt.usb-quirks $(1)/usr/share/cups/usb/
	$(INSTALL_DIR) $(1)/usr/share/licenses/captppd
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/LICENSE $(1)/usr/share/licenses/captppd/
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/libcapt-src/LICENSE $(1)/usr/share/licenses/captppd/libcapt-LICENSE
endef

$(eval $(call BuildPackage,captppd))
