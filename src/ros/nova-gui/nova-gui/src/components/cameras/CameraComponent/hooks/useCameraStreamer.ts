import { useSelector } from "react-redux";
import { useCameraStreamerActions } from "../../../../redux/actions/useCameraStreamerActions.ts";
import { CameraStreamerStatus } from "../../../../redux/models/CameraStreamState.ts";
import { RootState } from "../../../../redux/RootState.ts";
import { useCallback, useEffect } from "react";
import { ServerMessage } from "./serverMessages.ts";
import useWebSocket from "react-use-websocket";
import { cloneDeep } from "lodash";
import { useBifrost } from "../../../../redux/actions/bifrost/useBifrostAction.ts";
import { RosTopic } from "../../../../ros/topics/rosTopic.ts";

/**
 * Custom React hook for managing camera streaming functionality.
 * This hook handles communication with the server, synchronization with the ROS topic,
 * and updating the camera state based on received messages. Acts as an Orchestrator
 * handling addition and removal of cameras from cameras2.
 * @returns An object containing the `refreshAvailabilities` function.
 */
export const useCameraStreamer = () => {
  const cameraStreamerActions = useCameraStreamerActions();

  const bifrost = useBifrost({ topic: RosTopic.CAMERAS });

  useEffect(() => {
    bifrost.syncWithTopic();
     
  }, [bifrost]);

  const cameras = useSelector(
    (state: RootState) => state.cameraStreamerState.cameras
  );
  const roverIP = useSelector((state: RootState) => state.uiState.roverIP);

  const { sendJsonMessage, lastJsonMessage } = useWebSocket<ServerMessage>(
    `ws://${roverIP}:8443`,
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

  const refreshAvailabilities = useCallback(() => {
    sendJsonMessage({ type: "list" });
  }, [sendJsonMessage]);

  return {
    refreshAvailabilities,
  };
};
