import {
    Modal,
    ModalBody,
    ModalContent,
    // ModalFooter,
    ModalHeader,
    Tab,
    Tabs,
    // Tooltip
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

    const radioStatus = useSelector((state: RootState) => state.radioStore);
    const bifrost = useBifrost({ topic: RosTopic.RADIO_STATUS });

    useEffect(() => {
        bifrost.syncWithTopic();
    }, [bifrost]);

    const modalOpen = useSelector(
        (state: RootState) => state.uiState.radioStatusModalOpen
    );

    const onClose = () => uiActions.setRadioStatusModalOpen(false);



    let maxPoints = 30;

    const [allData, setData] = useState({
        time: [] as number[],
        signal: [] as number[],
        recv: [] as number[],
        sent: [] as number[],
        ping: [] as number[],
    });

    
    const addPoint = (currentData: number[], newValue: number) => {
        const newData = [...currentData, newValue];

        // Remove first element in the array if we exceed the maximum number of points
        if (newData.length > maxPoints) {
            newData.shift();
        }

        return newData;
    };

    // Update existing data
    useEffect(() => {
        if (radioStatus && radioStatus.stamp) {
            setData(allData => ({
                time: addPoint(allData.time, radioStatus.stamp.sec * 1000 + radioStatus.stamp.nanosec / 1_000_000),
                signal: addPoint(allData.signal, radioStatus.signal),
                recv: addPoint(allData.recv, radioStatus.recv),
                sent: addPoint(allData.sent, radioStatus.sent),
                ping: addPoint(allData.ping, radioStatus.ping),
            }));
        } else return;
    }, [radioStatus]);

    const radioData = {
        signal: { name: 'Signal strength', data: allData.signal.map((v, i) => [allData.time[i], v]) },
        recv: { name: 'Received bandwidth', data: allData.recv.map((v, i) => [allData.time[i], v]) },
        sent: { name: 'Sent bandwidth', data: allData.sent.map((v, i) => [allData.time[i], v]) },
        ping: { name: 'Ping', data: allData.ping.map((v, i) => [allData.time[i], v]) },
    };

    // const [maxPoints, setMaxPoints] = useState(300);

    // const timeLimit = (
    //     <Tooltip
    //         className="text-tiny text-default-500 rounded-md"
    //         content="Press Enter to confirm"
    //         placement="left"
    //     >
    //         <input
    //             aria-label="Seconds value"
    //             className="px-1 py-0.5 w-14 text-right text-small text-default-700 font-medium bg-default-100 outline-none transition-colors rounded-small border-medium border-transparent hover:border-primary focus:border-primary"
    //             type="number"
    //             value={maxPoints}
    //             onChange={(e: React.ChangeEvent<HTMLInputElement>) => {
    //                 const v = Number(e.target.value);
    //                 if (!isNaN(v)) setMaxPoints(v);
    //             }}
    //         />
    //     </Tooltip>
    // )


    return (
        <Modal
            isOpen={modalOpen}
            className="dark text-foreground"
            onClose={onClose}
            size="5xl"
        >
            <ModalContent>
                <ModalHeader className="flex flex-col gap-1">Radio data</ModalHeader>
                {/* <ModalFooter>Show last {timeLimit} seconds</ModalFooter> */}

                <ModalBody>
                    <Tabs
                        variant="underlined"
                        classNames={{
                            tabList: "gap-6 w-full relative rounded-none p-0 border-b border-divider",
                        }}
                    >
                        <Tab title="Signal">
                            <DataChart dataset={[radioData.signal]} chartOptions={ChartOptions(ChartStyle.Signal, maxPoints)} />
                        </Tab>

                        <Tab title="Received">
                            <DataChart dataset={[radioData.recv]} chartOptions={ChartOptions(ChartStyle.Received, maxPoints)} />
                        </Tab>

                        <Tab title="Sent">
                            <DataChart dataset={[radioData.sent]} chartOptions={ChartOptions(ChartStyle.Sent, maxPoints)} />
                        </Tab>

                        <Tab title="Ping">
                            <DataChart dataset={[radioData.ping]} chartOptions={ChartOptions(ChartStyle.Ping, maxPoints)} />
                        </Tab>
                    </Tabs>
                </ModalBody>
            </ModalContent>
        </Modal>
    );
};