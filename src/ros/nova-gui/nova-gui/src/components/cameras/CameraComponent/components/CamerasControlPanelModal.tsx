import {
  Button,
  Modal,
  ModalBody,
  ModalContent,
  ModalHeader,
  Table,
  TableBody,
  TableCell,
  TableColumn,
  TableHeader,
  TableRow,
  Tooltip,
} from "@nextui-org/react";
import { useBifrost } from "../../../../redux/actions/bifrost/useBifrostAction.ts";
import { RosTopic } from "../../../../ros/topics/rosTopic.ts";
import { useEffect } from "react";
import { useSelector } from "react-redux";
import { RootState } from "../../../../redux/RootState.ts";
import { RosService } from "../../../../ros/services/rosService.ts";
import { Pause, Play, ExternalLink } from "react-feather";
import { useRosNodes } from "../../../../utils/hooks/useRosNodes.ts";
import { BooleanChip } from "./BooleanChip.tsx";
import { allCams } from "../../../../views/shared/CamerasPage/CameraPageConstants.tsx";

export const CameraControlPanelModal = (props: {
  showModal: boolean;
  closeModal: () => void;
  refreshAvailabilies: () => void;
}) => {
  const bifrost = useBifrost({ topic: RosTopic.CAMERAS });

  const bifrostStarter = useBifrost({ service: RosService.START_CAMS });

  const bifrostStopper = useBifrost({ service: RosService.PAUSE_CAMS });

  const nodes = useRosNodes();

  const cameras2Running = nodes.includes("/camera_streamer");

  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  const startStreaming = () =>
    bifrostStarter.callService(
      { serials: [] },
      {
        responseToast: true,
        successToastMessage: "All Cameras Started Up!",
        handleResponse: () => props.refreshAvailabilies(),
      }
    );

  const pauseStreaming = () =>
    bifrostStopper.callService(
      { serials: [] },
      {
        responseToast: true,
        successToastMessage: "All Cameras Paused!",
        handleResponse: () => props.refreshAvailabilies(),
      }
    );

  return (
    <Modal
      isOpen={props.showModal}
      className="dark text-foreground"
      size="5xl"
      onClose={props.closeModal}
    >
      <ModalContent>
        <ModalHeader>Cameras 2 Control Panel</ModalHeader>
        <ModalBody>
          <div className="flex flex-row m-4 ml-0 gap-4 items-center justify-between">
            <div className="flex flex-row gap-4">
              <Button size="sm" color="primary" onPress={startStreaming}>
                <Play size="15px" fill="white" /> Start Streaming
              </Button>
              <Button size="sm" color="danger" onPress={pauseStreaming}>
                <Pause size="15px" fill="white" /> Pause Streaming
              </Button>
            </div>
            <Tooltip
              className="dark text-foreground"
              content="Not Real Time"
              closeDelay={100}
            >
              <BooleanChip
                boolean={cameras2Running}
                variant="dot"
                trueText="Cameras2 Running"
                falseText="Cameras2 Stopped"
                size="lg"
              />
            </Tooltip>
          </div>

          <CamerasTable refreshAvailabilies={props.refreshAvailabilies} />
        </ModalBody>
      </ModalContent>
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

  const bifrostStopper = useBifrost({ service: RosService.PAUSE_CAMS });

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
    bifrostStopper.callService(
      { serials: [cameraSerial] },
      {
        responseToast: true,
        successToastMessage: `${cameraSerial} Paused!`,
        errorToastMessage: `${cameraSerial} Failed to Pause!`,
        handleResponse: refreshAvailabilies,
      }
    );

  return (
    <Table
      removeWrapper
      isCompact
      className="overflow-scroll h-[45vh] overflow-x-hidden hide-scrollbar"
      isHeaderSticky
    >
      <TableHeader>
        <TableColumn>Serial</TableColumn>
        <TableColumn>Connection</TableColumn>
        <TableColumn>Status</TableColumn>
        <TableColumn align="end">
          <div className="flex flex-row justify-end">Actions</div>
        </TableColumn>
      </TableHeader>
      <TableBody
        emptyContent={"No Cameras Detected. Check if Cameras2 is running"}
      >
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
                  color="secondary"
                  onPress={() => window.open(
                      `/cameras/${serial}`,
                      "_blank",
                      "rel=noopener noreferrer"
                    )
                  }
                >
                  <ExternalLink size="15px" fill="white" />
                </Button>
                <Button
                  isIconOnly
                  size="sm"
                  color="primary"
                  disabled={!onlineCameraSerials.includes(serial)}
                  onPress={() => startStreaming(serial)}
                >
                  <Play size="15px" fill="white" />
                </Button>
                <Button
                  isIconOnly
                  size="sm"
                  color="danger"
                  disabled={!onlineCameraSerials.includes(serial)}
                  onPress={() => pauseStreaming(serial)}
                >
                  <Pause size="15px" fill="white" />
                </Button>
              </div>
            </TableCell>
          </TableRow>
        ))}
      </TableBody>
    </Table>
  );
};
