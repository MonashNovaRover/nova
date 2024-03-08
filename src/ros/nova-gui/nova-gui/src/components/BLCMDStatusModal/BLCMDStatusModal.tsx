import {
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
import { useBifrost } from "../../redux/actions/bifrost/useBifrostAction";
import { RosTopic } from "../../ros/topics/rosTopic";
import { useEffect } from "react";
import { useSelector } from "react-redux";
import { RootState } from "../../redux/RootState";
import { useUIActions } from "../../redux/actions/useUIActions";
import { BLCMD_INDEX } from "../../constants";
import { ComplainingChips } from "./ComplainingChips";

export const BLCMDStatusModal = () => {
  const bifrost = useBifrost({ topic: RosTopic.BLCMD_ERRORS });

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
      size="5xl"
      onClose={onClose}
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
                  <div className="flex flex-row justify-end">Actions</div>
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
                    <div className="flex flex-row gap-2 justify-end">Bro</div>
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
