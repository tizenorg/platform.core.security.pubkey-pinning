CONFIGURE_FILE(pubkey-pinning-popup.service.in pubkey-pinning-popup.service @ONLY)
CONFIGURE_FILE(pubkey-pinning-popup.socket.in pubkey-pinning-popup.socket @ONLY)

INSTALL(
	FILES
		pubkey-pinning-popup.service
		pubkey-pinning-popup.socket
	DESTINATION
		${SYSTEMD_UNIT_DIR}
	)
