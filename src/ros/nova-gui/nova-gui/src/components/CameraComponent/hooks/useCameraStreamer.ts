import { useSelector } from "react-redux";
import { useCameraStreamerActions } from "../../../redux/actions/useCameraStreamerActions";
import {
  Camera,
  CameraStreamerStatus,
} from "../../../redux/models/CameraStreamState";
import { RootState } from "../../../redux/RootState";
import { useEffect } from "react";
import { ServerMessage } from "./serverMessages";

export const useCameraStreamer = () => {
  const cameraStreamerActions = useCameraStreamerActions();

  const cameras = useSelector(
    (state: RootState) => state.cameraStreamerState.cameras
  );

  useEffect(() => {
    cameraStreamerActions.updateStatus(CameraStreamerStatus.CONNECTING);
    const ws = new WebSocket("ws://192.168.64.7:8443");

    ws.addEventListener("open", () => {
      cameraStreamerActions.updateStatus(CameraStreamerStatus.CONNECTED);
      ws.send(JSON.stringify({ type: "setPeerStatus", roles: ["listener"] }));
    });

    ws.addEventListener("error", () => {
      cameraStreamerActions.updateStatus(CameraStreamerStatus.DISCONNECTED);
    });

    ws.addEventListener("message", (event: MessageEvent) => {
      const message: ServerMessage = JSON.parse(event.data);
      switch (message.type) {
        case "welcome":
          ws.send(JSON.stringify({ type: "list" }));
          break;
        case "list":
          cameraStreamerActions.updateCameras(
            message.producers.map<Camera>((producer) => ({
              peerId: producer.id,
              serial: producer.meta.serial,
            }))
          );
          break;
        case "peerStatusChanged":
          let updatedCameras: Camera[];
          if (message.roles.includes("producer")) {
            updatedCameras = cameras.map<Camera>((camera) => {
              if (camera.peerId === message.peerId)
                return { peerId: message.peerId, serial: message.meta!.serial };
              else return camera;
            });
          } else {
            updatedCameras = cameras.filter(
              (camera) => camera.peerId !== message.peerId
            );
          }
          cameraStreamerActions.updateCameras(updatedCameras);
          break;
        default:
          throw new Error(`Unknown Message ${message}`);
      }
    });
  }, []);
};
