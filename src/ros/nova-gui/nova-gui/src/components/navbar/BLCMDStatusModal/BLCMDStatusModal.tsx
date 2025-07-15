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
import { useBifrost } from "../../../redux/actions/bifrost/useBifrostAction.ts";
import { RosTopic } from "../../../ros/topics/rosTopic.ts";
import { useEffect } from "react";
import { useSelector } from "react-redux";
import { RootState } from "../../../redux/RootState.ts";
import { useUIActions } from "../../../redux/actions/useUIActions.ts";
import { BLCMD_INDEX } from "../../../constants.ts";
import { ComplainingChips } from "./ComplainingChips.tsx";
import { ArrowCounterclockwise } from "react-bootstrap-icons";
import { RosService } from "../../../ros/services/rosService.ts";
import { IRosBlcmdInterfacesBlcmdResetRequestConst } from "../../../ros/rosTypes.ts";
import { ToolTipButton } from "../../shared/components/TooltipButton.tsx";

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

  const resetBLCMD = (id: number) =>
    bifrostReset.callService(
      {
        type: IRosBlcmdInterfacesBlcmdResetRequestConst.BLCMD,
        id: id,
      },
      {
        successToastMessage: `${BLCMD_INDEX[id]}'s BLCMD was Reset`,
      }
    );

  const zeroResolver = (id: number) =>
    bifrostReset.callService(
      {
        type: IRosBlcmdInterfacesBlcmdResetRequestConst.RESOLVER,
        id: id,
      },
      {
        successToastMessage: `${BLCMD_INDEX[id]}'s Resolver was Zero'd`,
      }
    );

  return (
    <Modal
      isOpen={modalOpen}
      className="dark text-foreground"
      onClose={onClose}
      size="5xl"
    >
      <ModalContent>
        <ModalHeader className="flex flex-col gap-1">Motor Status</ModalHeader>
        <ModalBody>
          <Table
            removeWrapper
            isCompact
            className="overflow-scroll h-[45vh] overflow-x-hidden hide-scrollbar"
            isHeaderSticky
            aria-label="BLCMD Status Table"
          >
            <TableHeader>
              <TableColumn>Motor</TableColumn>
              <TableColumn>Status</TableColumn>
              <TableColumn align="end">
                <div className="flex flex-row justify-end">Reset</div>
              </TableColumn>
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
                      <ToolTipButton
                        tooltipContent="Reset BLCMD"
                        isIconOnly
                        size="sm"
                        variant="flat"
                        color="danger"
                        onPress={() => resetBLCMD(blcmd.id)}
                        placement={undefined}
                      >
                        <ArrowCounterclockwise />
                      </ToolTipButton>

                      <ToolTipButton
                        tooltipContent="Zero Resolver"
                        isIconOnly
                        size="sm"
                        variant="flat"
                        color="danger"
                        onPress={() => zeroResolver(blcmd.id)}
                        placement={undefined}
                      >
                        123
                      </ToolTipButton>
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
