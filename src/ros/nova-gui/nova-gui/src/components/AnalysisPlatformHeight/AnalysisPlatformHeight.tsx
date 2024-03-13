/*
* Author: Connor Macdougall
* Displays height of analysis platform
*/

import { useSelector } from "react-redux";
import { RootState } from "../../redux/RootState";
import { useBifrost } from "../../redux/actions/bifrost/useBifrostAction";
import { RosTopic } from "../../ros/topics/rosTopic";
import { useEffect, useState } from "react";
import { Button, Card, CardBody, CardHeader, Modal, ModalContent, ModalHeader, ModalBody, useDisclosure } from "@nextui-org/react";
import { HelpCircle } from "react-feather";


const AnalysisPlatformHeight: React.FC = () => {
    const {isOpen, onOpen, onOpenChange} = useDisclosure();

    const tofStore = useSelector(
        (state: RootState) => state.tofStore
    );

    const bifrost = useBifrost({ topic: RosTopic.TOF });

    const [rangeMin, setRangeMin] = useState(0);
    const [rangeMax, setRangeMax] = useState(100);
    const [range, setRange] = useState(tofStore.range);

    useEffect(() => {
        bifrost.syncWithTopic();
        if (tofStore.header.frame_id == "analysis_platform") {
            setRange(tofStore.range);
            setRangeMin(tofStore.min_range);
            setRangeMax(tofStore.max_range);
        }
    }, [bifrost]);

    const tofReading = () => {
        if (range > rangeMax || range < rangeMin) {
            return <CardBody className="bg-rose-600 text-center p-3 text-3xl flex flex-row justify-around"><p>Error: {range}cm</p>
                <Button
                    isIconOnly
                    radius="sm"
                    size="sm"
                    variant="shadow"
                    onPress={onOpen}
                >
                    <HelpCircle className="w-4 h-4 " />
                </Button>
                <Modal isOpen={isOpen} onOpenChange={onOpenChange} className="dark text-foreground">
                    <ModalContent>
                    {() => (
                        <>
                        <ModalHeader className="flex flex-col gap-1 text-rose-600">Analysis Platform Height (TOF Sensor) Error</ModalHeader>
                        <ModalBody>
                            <p className="mb-2 text-lg">Error: Height received is out of given range of {rangeMin}cm to {rangeMax}cm (also sent from rover).</p>
                        </ModalBody>
                        </>
                    )}
                    </ModalContent>
                </Modal>
            </CardBody>;
        } else {
            return <CardBody className="bg-pink-600 text-center text-3xl p-3">{range}cm off Ground</CardBody>;
        }
    }

    return <Card className="w-64 m-4">
        <CardHeader className="text-h1 text-xl text-center p-3 m-0">
            Analysis Platform Height
        </CardHeader>
        {tofReading()}
    </Card>;
}

export default AnalysisPlatformHeight;