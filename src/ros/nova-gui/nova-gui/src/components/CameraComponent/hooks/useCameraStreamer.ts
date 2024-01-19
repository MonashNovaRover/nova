import { useSelector } from "react-redux";
import { useCameraStreamerActions } from "../../../redux/actions/useCameraStreamerActions";
import {
  Camera,
  CameraStreamerStatus,
} from "../../../redux/models/CameraStreamState";
import { RootState } from "../../../redux/RootState";
import { useCallback, useEffect } from "react";
import { ServerMessage } from "./serverMessages";
import useWebSocket from "react-use-websocket";

export const useCameraStreamer = () => {
  const cameraStreamerActions = useCameraStreamerActions();

  const cameras = useSelector(
    (state: RootState) => state.cameraStreamerState.cameras
  );

  const { sendJsonMessage, lastJsonMessage } = useWebSocket<ServerMessage>(
    "ws://192.168.0.4:8443",
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
    [sendJsonMessage]
  );

  useEffect(() => {
    if (!lastJsonMessage) return;
    switch (lastJsonMessage.type) {
      case "welcome":
        sendJsonMessage({ type: "list" });
        break;
      case "list":
        cameraStreamerActions.updateCameras(
          lastJsonMessage.producers.map<Camera>((producer) => ({
            peerId: producer.id,
            serial: producer.meta.serial,
          }))
        );
        break;
      case "peerStatusChanged":
        let updatedCameras: Camera[];
        if (lastJsonMessage.roles.includes("producer")) {
          updatedCameras = cameras.map<Camera>((camera) => {
            if (camera.peerId === lastJsonMessage.peerId)
              return {
                peerId: lastJsonMessage.peerId,
                serial: lastJsonMessage.meta!.serial,
              };
            else return camera;
          });
        } else {
          updatedCameras = cameras.filter(
            (camera) => camera.peerId !== lastJsonMessage.peerId
          );
        }
        cameraStreamerActions.updateCameras(updatedCameras);
        break;
      default:
        throw new Error(`Unknown Message ${lastJsonMessage}`);
    }
  }, [lastJsonMessage]);
};
