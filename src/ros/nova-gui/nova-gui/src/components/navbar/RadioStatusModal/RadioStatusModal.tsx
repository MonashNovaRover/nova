import {
    Modal,
    ModalBody,
    ModalContent,
    ModalHeader,
    Tab,
    Tabs,
} from "@nextui-org/react";

import { useEffect, useState } from "react";
import ReactApexChart from "react-apexcharts";
import { ChartOptions, ChartStyle } from "./ChartOptions.ts";
import { useSelector } from "react-redux";
import type { RootState } from "../../../redux/RootState";
import { useUIActions } from "../../../redux/actions/useUIActions.ts";

export const RadioStatusModal = () => {

    const uiActions = useUIActions();

    const radioStatus = useSelector((state: RootState) => state.radioStore);

    const modalOpen = useSelector(
        (state: RootState) => state.uiState.radioStatusModalOpen
    );

    const onClose = () => uiActions.setRadioStatusModalOpen(false);

    const maxPoints = 30;

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
                            <ReactApexChart
                                type="line"
                                options={ChartOptions(ChartStyle.Signal, radioData.signal.name, maxPoints)}
                                series={[radioData.signal]}
                            />
                        </Tab>

                        <Tab title="Received">
                            <ReactApexChart
                                type="line"
                                options={ChartOptions(ChartStyle.Received, radioData.recv.name, maxPoints)}
                                series={[radioData.recv]}
                            />
                        </Tab>

                        <Tab title="Sent">
                            <ReactApexChart
                                type="line"
                                options={ChartOptions(ChartStyle.Sent, radioData.sent.name, maxPoints)}
                                series={[radioData.sent]}
                            />
                        </Tab>

                        <Tab title="Ping">
                            <ReactApexChart
                                type="line"
                                options={ChartOptions(ChartStyle.Ping, radioData.ping.name, maxPoints)}
                                series={[radioData.ping]}
                            />
                        </Tab>

                    </Tabs>
                </ModalBody>
            </ModalContent>
        </Modal>
    );
};