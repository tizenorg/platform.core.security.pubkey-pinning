CONFIGURE_FILE(pubkey-pinning-popup.service.in pubkey-pinning-popup.service @ONLY)

INSTALL(
	FILES
		pubkey-pinning-popup.service
		pubkey-pinning-popup.socket
	DESTINATION
		${SYSTEMD_UNIT_DIR}
	)
