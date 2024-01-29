/**
 * Author: Connor Macdougall
 * This component is for the CCD inputs.
 * It sends requests to the 'raman_spectra' ROS service.
 */

import { Button, Card, CardHeader, Input, Modal, ModalBody, ModalContent, ModalHeader, useDisclosure } from "@nextui-org/react";
import { HelpCircle } from "react-feather";

const RamanCCDInputs: React.FC = () => {
    const {isOpen, onOpen, onOpenChange} = useDisclosure();

    return (
        <Card className="m-1 p-2 flex flex-row flex-1 space-x-2">
            <CardHeader className="shrink-0 w-24 p-1">CCD Inputs</CardHeader>
            <Button isIconOnly className="w-8 h-8 m-3" radius="md" onPress={onOpen}>
                <HelpCircle className="w-6 h-6" />
            </Button>
            <Modal className="dark text-foreground" isOpen={isOpen} onOpenChange={onOpenChange} isDismissable={false}>
                <ModalContent>
                {() => (
                    <>
                    <ModalHeader className="flex flex-col gap-1">CCD Inputs Help</ModalHeader>
                    <ModalBody>
                        <p>Specify the port name. The default value should work.</p>
                        <p>SH (SHift gate) period's minimum value is 20, its maximum value is 4294967295 and must be an integer.</p>
                        <p>ICG (Integration Clear Gate) period's minimum value is 14776, its maximum value is 4294967295 and must be an integer. The value for the ICG period <em className="text-xl font-black not-italic">MUST</em> be an integer multiple of the SH period.</p>
                        <p>Average determines the amount of samples taken and averaged by the firmware, its minimum value is 1 and its maximum value is 15.</p>
                    </ModalBody>
                    </>
                )}
                </ModalContent>
            </Modal>
            <Input className="shrink-0 w-44 grow" type="port" label="Port" placeholder="Example: /dev/ttyACM0" defaultValue="/dev/ttyACM0" />
            <Input className="shrink-0 w-36 grow" type="shperiod" label="SH Period" placeholder="[20, 4294967295]" defaultValue="200" />
            <Input className="shrink-0 w-40 grow" type="icgperiod" label="ICG Period" placeholder="[14776, 4294967295]" defaultValue="100000" />
            <Input className="shrink-0 w-20 grow" type="average" label="Average" placeholder="[1, 15]" defaultValue="1" />
            <Button className="bg-pink-600 h-14 flex flex-row shrink-0 grow w-80" radius="lg">Collect</Button>
        </Card>
    )
}

export default RamanCCDInputs;