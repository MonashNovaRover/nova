import {
  Button,
  Modal,
  ModalBody,
  ModalDialog,
  ModalHeader,
  Table,
  TableBody,
  TableCell,
  TableColumn,
  TableHeader,
  TableRow,
  Tooltip,
  useOverlayState,
} from "@heroui/react";
import { useBifrost } from "../../../redux/actions/bifrost/useBifrostAction.ts";
import { RosTopic } from "../../../ros/topics/rosTopic.ts";
import { useEffect } from "react";
import { useSelector } from "react-redux";
import { RootState } from "../../../redux/RootState.ts";
import { RosService } from "../../../ros/services/rosService.ts";
import { Pause, Play, Square, ExternalLink } from "react-feather";
import { useRosNodes } from "../../../utils/hooks/useRosNodes.ts";
import { BooleanChip } from "../CameraComponent/components/BooleanChip.tsx";
import { allCams } from "../../../views/shared/CamerasPage/CameraViewConstants.tsx";
import { useStreamingBifrost } from "../hooks/cameraBifrostHooks.ts";

// TODO: delete
export const CameraControlPanelModal = (props: {
  showModal: boolean;
  closeModal: () => void;
  refreshAvailabilies: () => void;
}) => {
  const overlayState = useOverlayState({ isOpen: props.showModal, onOpenChange: (isOpen) => !isOpen && props.closeModal() });
  const bifrost = useBifrost({ topic: RosTopic.CAMERAS });

  const nodes = useRosNodes();
  const camerasRunning = nodes.includes("/camera_streamer");

  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  const [startStreaming, pauseStreaming, stopStreaming] = useStreamingBifrost(props.refreshAvailabilies);
  const onlineCameras = useSelector((state: RootState) => state.camerasStore.cameras);
  const onlineCameraSerials = onlineCameras.map((cam) => cam.serial);

  return (
    <Modal state={overlayState}>
      <ModalDialog>
        <ModalHeader>Cameras Control Panel</ModalHeader>
        <ModalBody>
          <div className="flex flex-row m-4 ml-0 gap-4 items-center justify-between">
            <div className="flex flex-row gap-4">
              <Button size="sm" variant="primary" onPress={()=>startStreaming(onlineCameraSerials, true)}>
                <Play size="15px" fill="white"/> Start Streaming
              </Button>
              <Button size="sm" variant="secondary" onPress={()=>pauseStreaming(onlineCameraSerials, true)}>
                <Pause size="15px" fill="white" /> Pause Streaming
              </Button>
              <Button size="sm" variant="danger" onPress={()=>stopStreaming(onlineCameraSerials, true)}>
                <Square size="15px" fill="white" /> Stop Streaming
              </Button>
            </div>
            <Tooltip closeDelay={100}>
              <Tooltip.Trigger><BooleanChip
                boolean={camerasRunning}
                variant="dot"
                trueText="Cameras Running"
                falseText="Cameras Stopped"
                size="lg"
              /></Tooltip.Trigger>
              <Tooltip.Content>Not Real Time</Tooltip.Content>
            </Tooltip>
          </div>

          <CamerasTable refreshAvailabilies={props.refreshAvailabilies} />
        </ModalBody>
      </ModalDialog>
    </Modal>
  );
};

const CamerasTable = (props: { refreshAvailabilies: () => void }) => {
  const { refreshAvailabilies } = props;

  const onlineCameras = useSelector(
    (state: RootState) => state.camerasStore.cameras
  );

  const onlineCameraSerials = onlineCameras.map((cam) => cam.serial);

  const cameras = Array.from(new Set([...onlineCameraSerials, ...allCams]));

  const cameraStreamerMap = useSelector(
    (state: RootState) => state.cameraStreamerState.cameras
  );

  const bifrostStarter = useBifrost({ service: RosService.START_CAMS });

  const bifrostPauser = useBifrost({ service: RosService.PAUSE_CAMS });

  const bifrostStopper = useBifrost({ service: RosService.STOP_CAMS });

  const startStreaming = (cameraSerial: string) =>
    bifrostStarter.callService(
      { serials: [cameraSerial] },
      {
        responseToast: true,
        successToastMessage: `${cameraSerial} Started!`,
        errorToastMessage: `${cameraSerial} Failed to Start!`,
        handleResponse: refreshAvailabilies,
      }
    );

  const pauseStreaming = (cameraSerial: string) =>
    bifrostPauser.callService(
      { serials: [cameraSerial] },
      {
        responseToast: true,
        successToastMessage: `${cameraSerial} Paused!`,
        errorToastMessage: `${cameraSerial} Failed to Pause!`,
        handleResponse: refreshAvailabilies,
      }
    );
  const stopStreaming = (cameraSerial: string) =>
    bifrostStopper.callService(
      { serials: [cameraSerial] },
      {
        responseToast: true,
        successToastMessage: `${cameraSerial} Stopped!`,
        errorToastMessage: `${cameraSerial} Failed to Stop!`,
        handleResponse: refreshAvailabilies,
      }
    );

  return (
    <Table
      className="overflow-scroll h-[45vh] overflow-x-hidden hide-scrollbar"
    >
      <TableHeader>
        <TableColumn>Serial</TableColumn>
        <TableColumn>Connection</TableColumn>
        <TableColumn>Status</TableColumn>
        <TableColumn>
          <div className="flex flex-row justify-end">Actions</div>
        </TableColumn>
      </TableHeader>
      <TableBody>
        {cameras.map((serial) => (
          <TableRow>
            <TableCell>{serial}</TableCell>
            <TableCell>
              <BooleanChip
                boolean={onlineCameraSerials.includes(serial)}
                trueText="Online"
                falseText="Offline"
                variant="dot"
              />
            </TableCell>
            <TableCell>
              {onlineCameraSerials.includes(serial) ? (
                <BooleanChip
                  boolean={!!cameraStreamerMap[serial]}
                  trueText="Streaming"
                  falseText="Idle"
                  falseColor="primary"
                  variant="flat"
                />
              ) : (
                <BooleanChip
                  boolean={false}
                  falseText="Not Found"
                  falseColor="danger"
                  variant="flat"
                  trueText="Idle"
                />
              )}
            </TableCell>
            <TableCell>
              <div className="flex flex-row gap-2 justify-end">
                <Button
                  isIconOnly
                  size="sm"
                  variant="primary"
                  isDisabled={!onlineCameraSerials.includes(serial)}
                  onPress={() => startStreaming(serial)}
                >
                  <Play size="15px" fill="white" />
                </Button>
                <Button
                  isIconOnly
                  size="sm"
                  variant="secondary"
                  isDisabled={!onlineCameraSerials.includes(serial)}
                  onPress={() => pauseStreaming(serial)}
                >
                  <Pause size="15px" fill="white" />
                </Button>
                <Button
                  isIconOnly
                  size="sm"
                  variant="danger"
                  isDisabled={!onlineCameraSerials.includes(serial)}
                  onPress={() => stopStreaming(serial)}
                >
                  <Square size="15px" fill="white" />
                </Button>
                <Button
                  isIconOnly
                  size="sm"
                  variant="ghost"
                  onPress={() => window.open(
                    `/cameras/${serial}?autostart=true`,
                    "_blank",
                    "rel=noopener noreferrer"
                  )
                  }
                >
                  <ExternalLink size="15px" fill="white" />
                </Button>
              </div>
            </TableCell>
          </TableRow>
        ))}
      </TableBody>
    </Table>
  );
};
