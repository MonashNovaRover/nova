/*
* Author: Connor Macdougall
* Displays height of analysis platform
*/

import { useSelector } from "react-redux";
import { RootState } from "../../redux/RootState";
import { useBifrost } from "../../redux/actions/bifrost/useBifrostAction";
import { RosTopic } from "../../ros/topics/rosTopic";
import { useEffect } from "react";
import { Button, Card, CardBody, CardHeader, Modal, ModalContent, ModalHeader, ModalBody, useDisclosure, CardProps } from "@nextui-org/react";
import { HelpCircle } from "react-feather";

interface TOFHeightProps extends CardProps {}

const TOFHeight: React.FC<TOFHeightProps> = (props) => {
    const {isOpen, onOpen, onOpenChange} = useDisclosure();

    const tofStore = useSelector(
        (state: RootState) => state.tofStore
    );

    const bifrost = useBifrost({ topic: RosTopic.TOF });

    const range = tofStore.range;
    const rangeMin = tofStore.min_range;
    const rangeMax = tofStore.max_range;

    const outOfRange = range < rangeMin;
    const dangerRange = range < 5;
    const warningRange = range < 15;
   

    useEffect(() => {
        bifrost.syncWithTopic();
    }, [bifrost]);


    const infoButton = () => {
        return (
            <Button
                isIconOnly
                radius="sm"
                size="sm"
                variant="shadow"
                onPress={onOpen}
            >
                <HelpCircle className="w-4 h-4 " />
            </Button>
        );
    }

    const infoModal = () => {
        return (
            <Modal isOpen={isOpen} onOpenChange={onOpenChange} className="dark text-foreground">
                <ModalContent>
                {() => (
                    <>
                    <ModalHeader className="flex flex-col gap-1 text-rose-600">Height (TOF Sensor) Error</ModalHeader>
                    <ModalBody>
                        <p className="mb-2 text-lg">Error: Height received is out of given range of {rangeMin} mm to {rangeMax} mm (also sent from rover).</p>
                    </ModalBody>
                    </>
                )}
                </ModalContent>
            </Modal>
        )
    }
    

    const tofReading = () => {
        return (
            <CardBody className={`${outOfRange || dangerRange ? "bg-danger" :  warningRange ? "bg-orange-500" : "bg-default"} rounded-lg text-xl p-3 flex flex-row justify-center`}>
                {
                    !outOfRange ? 
                        <p>Height: {range} mm</p> : 
                        <p>Error: {range} mm</p>                
                }
            </CardBody>
        )
    }

    return (
        <Card {...props}>
            <CardHeader className="text-h1 h-12 p-3 m-0 justify-between">
                Height
                {outOfRange && infoButton()}
                {infoModal()}
            </CardHeader>
            <CardBody>
                {tofReading()}
            </CardBody>
        </Card>
    );
}

export default TOFHeight;