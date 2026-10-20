import { Modal, ModalBody, ModalContent, ModalHeader } from "@heroui/react";

import { useEffect } from "react";
import { useSelector } from "react-redux";
import type { RootState } from "../../../redux/RootState";
import { useUIActions } from "../../../redux/actions/useUIActions.ts";

import BatteryChart from "./BatteryChart.tsx";

export const BatteryModal = () => {

    const uiActions = useUIActions();

    const batteryStatus = useSelector(
        (state:RootState) => state.batteryStore
    );

    const modalOpen = useSelector(
        (state: RootState) => state.uiState.batteryModalOpen
    );

    const onClose = () => uiActions.setBatteryModalOpen(false);

    return (
        <Modal
            isOpen={modalOpen}
            className="dark text-foreground"
            onClose={onClose}
            size="5xl"
        >
            <ModalContent>
                <ModalHeader className="flex flex-col gap-1"></ModalHeader>
                <ModalBody>
                    <p>Battery Status: {batteryStatus.percentage}%</p>
                    <BatteryChart/>
                </ModalBody>
            </ModalContent>
        </Modal>
    );
}