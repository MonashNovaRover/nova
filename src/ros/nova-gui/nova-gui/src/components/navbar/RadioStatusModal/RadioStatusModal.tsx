import {
    Modal,
    ModalBody,
    ModalContent,
    ModalHeader,
    Tab,
    Tabs,
    Button,
    Input,
    Popover,
    PopoverTrigger,
    PopoverContent,
    Dropdown,
    DropdownTrigger,
    DropdownMenu,
    DropdownItem
} from "@nextui-org/react";

import { useEffect, useState } from "react";
import ReactApexChart from "react-apexcharts";
import { Settings } from "react-feather";
import { ChartOptions, ChartStyle } from "./ChartOptions.ts";
import { useSelector } from "react-redux";
import type { RootState } from "../../../redux/RootState";
import { useUIActions } from "../../../redux/actions/useUIActions.ts";

type RadioStatusModalProps = {
    rosTimeout: number;
    setRosTimeout: (ms: number) => void;
};

export const RadioStatusModal = ({ rosTimeout, setRosTimeout }: RadioStatusModalProps) => {
    const uiActions = useUIActions();

    const radioStatus = useSelector((state: RootState) => state.radioStore);

    const modalOpen = useSelector(
        (state: RootState) => state.uiState.radioStatusModalOpen
    );

    const onClose = () => uiActions.setRadioStatusModalOpen(false);

    const [windowSize, setWindowSize] = useState(30);

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
        if (newData.length > windowSize) {
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

    const [windowSizeInput, setWindowSizeInput] = useState(String(windowSize));
    const [rosTimeoutInput, setRosTimeoutInput] = useState(String(rosTimeout));

    const submitSettings = () => {
        // Clamp input
        const newWindow = Math.min(300, Math.max(5, Math.round(Number(windowSizeInput))));
        const newTimeout = Math.min(60000, Math.max(1000, Math.round(Number(rosTimeoutInput))));
        setWindowSizeInput(String(newWindow));
        setRosTimeoutInput(String(newTimeout));

        // Trim data if window decreased
        if (newWindow < windowSize) {
            setData((allData) => ({
                time: allData.time.slice(-newWindow),
                signal: allData.signal.slice(-newWindow),
                recv: allData.recv.slice(-newWindow),
                sent: allData.sent.slice(-newWindow),
                ping: allData.ping.slice(-newWindow),
            }));
        }

        setWindowSize(newWindow);
        setRosTimeout(newTimeout);
    };

    return (
        <Modal
            isOpen={modalOpen}
            className="dark text-foreground"
            onClose={onClose}
            size="5xl"
        >
            <ModalContent>
                <ModalHeader className="flex flex-col gap-1">
                    <div className="flex items-center gap-2">
                        Radio data
                        <Popover placement="bottom-start">
                            <PopoverTrigger>
                                <Button
                                    isIconOnly
                                    size="sm"
                                    variant="light"
                                >
                                    <Settings size={16} />
                                </Button>
                            </PopoverTrigger>

                            <PopoverContent className="w-[360px] dark text-foreground">
                                <div className="flex w-[355px] flex-col gap-4 p-3">
                                    <Input
                                        label="History window (s) "
                                        type="number"
                                        step="1"
                                        placeholder="5-300"
                                        value={windowSizeInput}
                                        onValueChange={setWindowSizeInput}
                                        isInvalid={!windowSizeInput}
                                        variant="bordered"
                                    />

                                    <Input
                                        label="ROS timeout (ms) "
                                        type="number"
                                        step="1"
                                        placeholder="1000-60000"
                                        value={rosTimeoutInput}
                                        onValueChange={setRosTimeoutInput}
                                        isInvalid={!rosTimeoutInput}
                                        variant="bordered"
                                    />

                                    <div className="flex flex-row items-center justify-between">
                                        <div className="flex flex-wrap gap-2">
                                            <Dropdown className="dark text-foreground">
                                                <DropdownTrigger>
                                                    <Button size="sm">Manage radios</Button>
                                                </DropdownTrigger>
                                                <DropdownMenu>
                                                    <DropdownItem
                                                        description="10.0.1.11"
                                                        key="base-bullet"
                                                        onPress={() => window.open('https://10.0.1.11', "_blank", "rel=noopener noreferrer")}>
                                                        Base Bullet
                                                    </DropdownItem>
                                                    <DropdownItem
                                                        description="10.0.1.10"
                                                        key="rover-bullet"
                                                        onPress={() => window.open('https://10.0.1.10', "_blank", "rel=noopener noreferrer")}>
                                                        Rover Bullet
                                                    </DropdownItem>
                                                </DropdownMenu>
                                            </Dropdown>
                                        </div>
                                        <Button
                                            size="sm"
                                            color="success"
                                            variant="flat"
                                            onPress={submitSettings}>
                                            Submit
                                        </Button>
                                    </div>
                                </div>
                            </PopoverContent>
                        </Popover>
                    </div>
                </ModalHeader>
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
                                options={ChartOptions(ChartStyle.Signal, radioData.signal.name, windowSize)}
                                series={[radioData.signal]}
                            />
                        </Tab>

                        <Tab title="Received">
                            <ReactApexChart
                                type="line"
                                options={ChartOptions(ChartStyle.Received, radioData.recv.name, windowSize)}
                                series={[radioData.recv]}
                            />
                        </Tab>

                        <Tab title="Sent">
                            <ReactApexChart
                                type="line"
                                options={ChartOptions(ChartStyle.Sent, radioData.sent.name, windowSize)}
                                series={[radioData.sent]}
                            />
                        </Tab>

                        <Tab title="Ping">
                            <ReactApexChart
                                type="line"
                                options={ChartOptions(ChartStyle.Ping, radioData.ping.name, windowSize)}
                                series={[radioData.ping]}
                            />
                        </Tab>

                    </Tabs>
                </ModalBody>
            </ModalContent>
        </Modal>
    );
};