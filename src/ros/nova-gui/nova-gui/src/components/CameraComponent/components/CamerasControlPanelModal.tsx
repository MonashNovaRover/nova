import {
  Chip,
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
} from "@nextui-org/react";
import { useBifrost } from "../../../redux/actions/bifrost/useBifrostAction";
import { RosTopic } from "../../../ros/topics/rosTopic";
import { useEffect } from "react";
import { useSelector } from "react-redux";
import { RootState } from "../../../redux/RootState";

export const CameraControlPanelModal = (props: {
  showModal: boolean;
  closeModal: () => void;
}) => {
  const bifrost = useBifrost({ topic: RosTopic.CAMERAS });

  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  const cameras = useSelector((state: RootState) => state.camerasStore.cameras);

  const cameraSerials = cameras.map((cam) => cam.serial);

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
          <Table removeWrapper>
            <TableHeader>
              <TableColumn>Serial</TableColumn>
              <TableColumn>Status</TableColumn>
            </TableHeader>
            <TableBody
              emptyContent={"No Cameras Detected. Check if Cameras2 is running"}
            >
              {cameras.map((cam) => (
                <TableRow>
                  <TableCell>{cam.serial}</TableCell>
                  <TableCell>
                    {cameraSerials.includes(cam.serial) ? (
                      <Chip color="success" size="sm">
                        Online
                      </Chip>
                    ) : (
                      <Chip color="danger" size="sm">
                        Offline
                      </Chip>
                    )}
                  </TableCell>
                </TableRow>
              ))}
              {/* {cameras.length === 0
                ? []
                : allCams.map((cam) => (
                    <TableRow>
                      <TableCell>{cam}</TableCell>
                      <TableCell>
                        {cameraSerials.includes(cam) ? (
                          <Chip color="success" size="sm">
                            Online
                          </Chip>
                        ) : (
                          <Chip color="danger" size="sm">
                            Offline
                          </Chip>
                        )}
                      </TableCell>
                    </TableRow>
                  ))} */}
            </TableBody>
          </Table>
        </ModalBody>
      </ModalContent>
    </Modal>
  );
};
