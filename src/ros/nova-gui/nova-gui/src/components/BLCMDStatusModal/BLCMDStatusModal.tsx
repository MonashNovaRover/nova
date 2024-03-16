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
import { useBifrost } from "../../redux/actions/bifrost/useBifrostAction";
import { RosTopic } from "../../ros/topics/rosTopic";
import { useEffect } from "react";
import { useSelector } from "react-redux";
import { RootState } from "../../redux/RootState";
import { useUIActions } from "../../redux/actions/useUIActions";
import { BLCMD_INDEX } from "../../constants";
import { ComplainingChips } from "./ComplainingChips";
import { ArrowCounterclockwise } from "react-bootstrap-icons";
import { RosService } from "../../ros/services/rosService";
import { IRosCoreBlcmdResetRequestConst } from "../../ros/rosTypes";

export const BLCMDStatusModal = () => {
  const bifrost = useBifrost({ topic: RosTopic.BLCMD_ERRORS });

  const bifrostReset = useBifrost({ service: RosService.BLCMD_RESET });

  const uiActions = useUIActions();

  useEffect(() => {
    bifrost.syncWithTopic();
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  const blcmdStatuses = useSelector(
    (state: RootState) => state.blcmdStatusStore.blcmds
  );

  const modalOpen = useSelector(
    (state: RootState) => state.uiState.blcmdStatusModalOpen
  );

  const onClose = () => uiActions.setBlcmdStatusModalOpen(false);

  return (
    <Modal
      isOpen={modalOpen}
      className="dark text-foreground"
      onClose={onClose}
      size="3xl"
    >
      <ModalContent>
        <ModalHeader className="flex flex-col gap-1">Motor Status</ModalHeader>
        <ModalBody>
          <Table
            removeWrapper
            isCompact
            className="overflow-scroll h-[45vh] overflow-x-hidden hide-scrollbar"
            isHeaderSticky
          >
            <TableHeader>
              <TableHeader>
                <TableColumn>Motor</TableColumn>
                <TableColumn>Status</TableColumn>
                <TableColumn align="end">
                  <div className="flex flex-row justify-end">Reset</div>
                </TableColumn>
              </TableHeader>
            </TableHeader>
            <TableBody>
              {blcmdStatuses.map((blcmd) => (
                <TableRow key={blcmd.id}>
                  <TableCell>{BLCMD_INDEX[blcmd.id]}</TableCell>
                  <TableCell>
                    <ComplainingChips {...blcmd} />
                  </TableCell>
                  <TableCell>
                    <div className="flex flex-row gap-2 justify-end">
                      <Tooltip
                        className="dark text-foreground"
                        content="Reset BLCMD"
                        color="danger"
                      >
                        <Button
                          isIconOnly
                          size="sm"
                          variant="flat"
                          color="danger"
                          onClick={() =>
                            bifrostReset.callService(
                              {
                                type: IRosCoreBlcmdResetRequestConst.BLCMD,
                                id: blcmd.id,
                              },
                              {
                                successToastMessage: `${
                                  BLCMD_INDEX[blcmd.id]
                                }'s BLCMD was Reset`,
                              }
                            )
                          }
                        >
                          <ArrowCounterclockwise />
                        </Button>
                      </Tooltip>
                      <Tooltip
                        className="dark text-foreground"
                        content="Zero Resolver"
                        color="danger"
                      >
                        <Button
                          isIconOnly
                          size="sm"
                          variant="flat"
                          color="danger"
                          onClick={() =>
                            bifrostReset.callService(
                              {
                                type: IRosCoreBlcmdResetRequestConst.RESOLVER,
                                id: blcmd.id,
                              },
                              {
                                successToastMessage: `${
                                  BLCMD_INDEX[blcmd.id]
                                }'s Resolver was Zero'd`,
                              }
                            )
                          }
                        >
                          123
                        </Button>
                      </Tooltip>
                    </div>
                  </TableCell>
                </TableRow>
              ))}
            </TableBody>
          </Table>
        </ModalBody>
      </ModalContent>
    </Modal>
  );
};
