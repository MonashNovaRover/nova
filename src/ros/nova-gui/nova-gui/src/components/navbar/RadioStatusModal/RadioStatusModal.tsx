import {
    Modal,
    ModalBody,
    ModalContent,
    ModalHeader,
} from "@nextui-org/react";

import { useEffect, useState } from "react";
import DataChart from "./DataChart.tsx";
import { ChartOptions, ChartStyle } from "./ChartOptions.ts";
import { useSelector } from "react-redux";
import type { RootState } from "../../../redux/RootState";
import { RosTopic } from "../../../ros/topics/rosTopic";
import { useBifrost } from "../../../redux/actions/bifrost/useBifrostAction";
import { useUIActions } from "../../../redux/actions/useUIActions.ts";

export const RadioStatusModal = () => {

    const uiActions = useUIActions();

    const radioStatus = useSelector((state: RootState) => state.radioStatusStore);
    const bifrost = useBifrost({ topic: RosTopic.RADIO_STATUS });

    useEffect(() => {
        bifrost.syncWithTopic();
    }, [bifrost]);

    const modalOpen = useSelector(
        (state: RootState) => state.uiState.radioStatusModalOpen
    );

    const onClose = () => uiActions.setRadioStatusModalOpen(false);




    const maxPoints = 360;
    const [signalData, setSignalData] = useState<number[]>([]);

    useEffect(() => {
        const updatedSignalData = signalData.concat(radioStatus.signal);
        if (updatedSignalData.length > maxPoints) {
            updatedSignalData.shift();
        }
        setSignalData(updatedSignalData);
    }, [radioStatus.recv,
        radioStatus.sent,
        radioStatus.signal,
        radioStatus.ping
    ]);

    const radioData = {
        signal: { name: 'Signal (dBm)', data: signalData.map((value, index) => [index, value]) },
        // recv: { name: 'Recv (kbps)', data: recvData.map((value, index) => [index, value]) },
        // sent: { name: 'Sent (kbps)', data: sentData.map((value, index) => [index, value]) },
        // ping: { name: 'Ping (ms)', data: pingData.map((value, index) => [index, value]) },
    };

    return (
        <Modal
            isOpen={modalOpen}
            className="dark text-foreground"
            onClose={onClose}
            size="5xl"
        >
            <ModalContent>
                <ModalHeader className="flex flex-col gap-1">Radio data</ModalHeader>
                <ModalBody>
                    <DataChart dataset={[radioData.signal]} chartOptions={ChartOptions(ChartStyle.Default)} />
                    {/* <DataChart dataset={[radioData.ping]} chartOptions={ChartOptions(ChartStyle.Default)} /> */}
                </ModalBody>
            </ModalContent>
        </Modal>
    );
};