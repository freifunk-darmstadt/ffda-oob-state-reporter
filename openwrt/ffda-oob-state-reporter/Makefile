include $(TOPDIR)/rules.mk

PKG_NAME:=ffda-oob-state-reporter
PKG_VERSION:=$(shell git show -s --format=%cd --date=short)
PKG_RELEASE:=1

PKG_BUILD_PARALLEL:=1

PKG_FILE_DEPENDS:=$(CURDIR)/../..

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	cp -R $(CURDIR)/../../src/* $(PKG_BUILD_DIR)
endef

define Package/ffda-oob-state-reporter
  SECTION:=net
  CATEGORY:=Network
  DEPENDS:=+libubox +libubus +libblobmsg-json
  TITLE:=OOB Device state reporter
endef

define Package/ffda-oob-state-reporter/conffiles
/etc/config/ffda-oob-state-reporter
endef

define Package/ffda-oob-state-reporter/install
	$(INSTALL_DIR) $(1)/sbin $(1)/etc/config $(1)/etc/init.d

	$(INSTALL_BIN) $(PKG_BUILD_DIR)/reporter $(1)/sbin/ffda-oob-state-reporter

	$(INSTALL_BIN) ./files/ffda-oob-state-reporter.init $(1)/etc/init.d/ffda-oob-state-reporter
	$(CP) ./files/ffda-oob-state-reporter.uci $(1)/etc/config/ffda-oob-state-reporter
endef

$(eval $(call BuildPackage,ffda-oob-state-reporter))