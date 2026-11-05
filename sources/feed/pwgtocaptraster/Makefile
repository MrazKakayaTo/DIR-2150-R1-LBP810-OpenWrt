include $(TOPDIR)/rules.mk

PKG_NAME:=pwgtocaptraster
PKG_VERSION:=0.1.0
PKG_RELEASE:=1
PKG_LICENSE:=MIT

include $(INCLUDE_DIR)/package.mk

define Package/pwgtocaptraster
  SECTION:=net
  CATEGORY:=Network
  SUBMENU:=Printing
  TITLE:=Streaming PWG Raster to monochrome CAPT CUPS raster
  DEPENDS:=+libcups
endef

define Package/pwgtocaptraster/description
  Minimal grayscale conversion filter used by Windows IPP Class Driver
  with the Canon LBP-810 captppd CUPS backend. A4/600dpi only.
endef

define Build/Prepare
	$(INSTALL_DIR) $(PKG_BUILD_DIR)
	$(CP) ./pwgtocaptraster.c $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(TARGET_CC) $(TARGET_CFLAGS) $(TARGET_CPPFLAGS) -Wall -Wextra \
		-o $(PKG_BUILD_DIR)/pwgtocaptraster \
		$(PKG_BUILD_DIR)/pwgtocaptraster.c $(TARGET_LDFLAGS) -lcups
endef

define Package/pwgtocaptraster/install
	$(INSTALL_DIR) $(1)/usr/lib/cups/filter
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/pwgtocaptraster $(1)/usr/lib/cups/filter/
	$(INSTALL_DIR) $(1)/usr/share/cups/mime
	$(INSTALL_DATA) ./pwgtocapt.convs $(1)/usr/share/cups/mime/
endef

$(eval $(call BuildPackage,pwgtocaptraster))
