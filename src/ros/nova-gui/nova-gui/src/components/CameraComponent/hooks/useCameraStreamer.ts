import { useSelector } from "react-redux";
import { useCameraStreamerActions } from "../../../redux/actions/useCameraStreamerActions";
import { CameraStreamerStatus } from "../../../redux/models/CameraStreamState";
import { RootState } from "../../../redux/RootState";
import { useCallback, useEffect } from "react";
import { ServerMessage } from "./serverMessages";
import useWebSocket from "react-use-websocket";
import { cloneDeep } from "lodash";

export const useCameraStreamer = () => {
  const cameraStreamerActions = useCameraStreamerActions();

  const cameras = useSelector(
    (state: RootState) => state.cameraStreamerState.cameras
  );

  const { sendJsonMessage, lastJsonMessage } = useWebSocket<ServerMessage>(
    "ws://192.168.64.7:8443",
    {
      onOpen: () => {
        sendPeerStatusMessage(CameraStreamerStatus.CONNECTED);
      },
      onError: () => {
        sendPeerStatusMessage(CameraStreamerStatus.DISCONNECTED);
      },
    }
  );

  const sendPeerStatusMessage = useCallback(
    (cameraStreamerStatus: CameraStreamerStatus) => {
      cameraStreamerActions.updateStatus(cameraStreamerStatus);
      sendJsonMessage({ type: "setPeerStatus", roles: ["listener"] });
    },
    // eslint-disable-next-line react-hooks/exhaustive-deps
    [sendJsonMessage]
  );

  useEffect(() => {
    if (!lastJsonMessage) return;
    switch (lastJsonMessage.type) {
      case "welcome":
        sendJsonMessage({ type: "list" });
        break;
      case "list": {
        const cameras: { [serial: string]: string } = {};
        lastJsonMessage.producers.forEach((producer) => {
          cameras[producer.meta.serial] = producer.id;
        });
        cameraStreamerActions.updateCameras(cameras);
        break;
      }

      case "peerStatusChanged": {
        const updatedCameras = cloneDeep(cameras);
        if (lastJsonMessage.meta)
          if (lastJsonMessage.roles.includes("producer")) {
            updatedCameras[lastJsonMessage.meta!.serial] =
              lastJsonMessage.peerId;
          } else {
            delete updatedCameras[lastJsonMessage.meta!.serial];
          }
        cameraStreamerActions.updateCameras(updatedCameras);
        break;
      }

      default:
        throw new Error(`Unknown Message ${lastJsonMessage}`);
    }
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [lastJsonMessage]);

  const refreshAvailability = useCallback(() => {
    sendJsonMessage({ type: "list" });
  }, [sendJsonMessage]);

  return {
    refreshAvailability,
  };
};
