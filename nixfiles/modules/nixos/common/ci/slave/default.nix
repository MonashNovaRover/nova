{ config, pkgs, lib, ... }:

let
  cfg = config.nova.ci.slave;
in
{
  options.nova.ci.slave = {
    enable = lib.mkEnableOption "CI slave services";

    nat_holepunch = {
      enable = lib.mkEnableOption "Enable wireguard nat holepunch";
      remoteIp = lib.mkOption {
        type = with lib.types; str;
        description = lib.mdDoc "The ip address or hostname of the remote.";
        default = "hydra.novarover.space";
      };
      remoteVpnIp = lib.mkOption {
        type = with lib.types; str;
        description = lib.mdDoc "The ip address of the remote within the vpn.";
        default = "10.0.126.1";
      };
      localVpnIp = lib.mkOption {
        type = with lib.types; str;
        description = lib.mdDoc "The ip address of the local computer within the vpn.";
        default = "10.0.126.2";
      };
      localVpnIfName = lib.mkOption {
        type = with lib.types; str;
        description = lib.mdDoc "The interface name to use for the wireguard network.";
        default = "wg_holepunch";
      };
    };

    remotePubSSHKey = lib.mkOption {
        type = with lib.types; str;
        description = lib.mdDoc "SSH pub key of the nixbuild user on the master.";
        default = "ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIC0Uce72ktg6CcPeMb94XhukecdBmuTnsUVyb2R/+wDe root@ci";
      };

  };

  config = lib.mkIf cfg.enable {
    nova.ci.common.enable = true;

    users.users.remotebuild = {
      isNormalUser = true;
      createHome = false;
      group = "remotebuild";
      openssh.authorizedKeys.keys = [ cfg.remotePubSSHKey ];
    };

    users.groups.remotebuild = {};
    nix.settings.trusted-users = [ "remotebuild" ];

    # Stupid nat punching:
    # TODO: port forward instead? - Can't access oracle dashboard to port forward on oracle side, port forwarding from workshop side didn't seem to work properly, pings wouldn't go through.
    systemd.timers."hydra-wg-holepunch" = lib.mkIf cfg.nat_holepunch.enable {
      wantedBy = [ "timers.target" ];
      timerConfig = {
        OnBootSec = "15m";
        OnUnitInactiveSec = "15m";
        Unit = "hydra-wg-holepunch.service";
      };
    };
    systemd.services."hydra-wg-holepunch" = lib.mkIf cfg.nat_holepunch.enable {
    script = ''
#!/bin/sh
# Use ssh as a side channel to create an ad-hoc wireguard tunnel between two hosts
# Ideally this will get through firewalls and NAT.

set -euo pipefail

SSHOPS="-i ~/.ssh/nova-oracle.key"
TARGET=${cfg.nat_holepunch.remoteIp}
USER=root

# Change these for every pair of computers you're connecting
_VPN_THEIR_IP=${cfg.nat_holepunch.remoteVpnIp}
_VPN_MY_IP=${cfg.nat_holepunch.localVpnIp}
WG_IF_NAME=${cfg.nat_holepunch.localVpnIfName}

WG=${pkgs.wireguard-tools}/bin/wg
IP=${pkgs.iproute2}/bin/ip
BASH=${pkgs.bash}/bin/bash

if ${pkgs.iputils}/bin/ping -c 1 $_VPN_THEIR_IP -W 2; then
  echo Able to ping remote over wireguard already. Exiting..
  exit 0
fi

SSH_CMD="${pkgs.openssh}/bin/ssh $SSHOPS $USER@$TARGET"

# change this if google kills this server ig.
STUN_SERVER=stun.l.google.com
STUN_PORT=19302

VPN_THEIR_IP=$_VPN_THEIR_IP/32
VPN_MY_IP=$_VPN_MY_IP/32

# sanity check

# clear existing connection
$SSH_CMD ip link del $WG_IF_NAME || :
$IP link del $WG_IF_NAME || :

echo -n "Find my public ip and port... "
MY_PUB_IP_PORT=$(${pkgs.stuntman}/bin/stunclient $STUN_SERVER $STUN_PORT | grep "Mapped address" | cut -d" " -f3)
MY_PUB_IP=$(echo $MY_PUB_IP_PORT | cut -d: -f1)
MY_PUB_PORT=$(echo $MY_PUB_IP_PORT | cut -d: -f2) # may not be reliable
echo $MY_PUB_IP $MY_PUB_PORT


echo -n "Find their public ip and port... "
THEIR_PUB_IP_PORT=$($SSH_CMD "nix-shell -p stuntman --command \"stunclient $STUN_SERVER $STUN_PORT\""| grep "Mapped address" | cut -d" " -f3)
THEIR_PUB_IP=$(echo $THEIR_PUB_IP_PORT | cut -d: -f1)
THEIR_PUB_PORT=$(echo $THEIR_PUB_IP_PORT | cut -d: -f2) # may not be reliable
echo $THEIR_PUB_IP $THEIR_PUB_PORT

echo Generating keys
MY_PRIV_KEY=$($WG genkey)
MY_PUB_KEY=$(echo $MY_PRIV_KEY|$WG pubkey)
THEIR_PRIV_KEY=$($WG genkey)
THEIR_PUB_KEY=$(echo $THEIR_PRIV_KEY|$WG pubkey)

send_udp()
{
  DATA=$1
  DST_IP=$2
  DST_PORT=$3
  SRC_PORT=$4
  ${pkgs.python3}/bin/python3 -c "
import socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((\"\", $SRC_PORT))
sock.sendto(b'$DATA', ('$DST_IP', $DST_PORT))
sock.close()"
}

echo Open a hole for remote...
send_udp hello $THEIR_PUB_IP $THEIR_PUB_PORT $MY_PUB_PORT

echo Configure local wireguard
$IP link add $WG_IF_NAME type wireguard
$BASH -c "$WG set $WG_IF_NAME listen-port $MY_PUB_PORT \
  private-key <(echo $MY_PRIV_KEY) peer $THEIR_PUB_KEY \
  endpoint $THEIR_PUB_IP:$THEIR_PUB_PORT allowed-ips $VPN_THEIR_IP \
  persistent-keepalive 11" # prime

echo configure remote wireguard
$SSH_CMD ip link add $WG_IF_NAME type wireguard
$SSH_CMD bash -c "\"wg set $WG_IF_NAME listen-port $THEIR_PUB_PORT \
  private-key <(echo $THEIR_PRIV_KEY) peer $MY_PUB_KEY \
  endpoint $MY_PUB_IP:$MY_PUB_PORT allowed-ips $VPN_MY_IP \
  persistent-keepalive 13\"" # different prime

echo set up routing
# set the link up
$IP link set up $WG_IF_NAME
$SSH_CMD ip link set up $WG_IF_NAME
# Give ourself an ip addr and a route to the remote
$IP route add $VPN_THEIR_IP dev $WG_IF_NAME
$IP addr add $VPN_MY_IP dev $WG_IF_NAME
# Give remote an ip addr and a route to us
$SSH_CMD ip route add $VPN_MY_IP dev $WG_IF_NAME
$SSH_CMD ip addr add $VPN_THEIR_IP dev $WG_IF_NAME
'';
};

    assertions = [
      {
        assertion = !config.nova.ci.master.enable;
        message = "The CI master and slave modules cannot be enabled simultaneously.";
      }
    ];
  };
}
