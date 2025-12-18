/**
 * Author: Connor Macdougall
 * This component is for the CCD inputs.
 * It sends requests to the 'raman_spectra' ROS service.
 */

import { Button, Card, CardHeader, Input, Modal, ModalBody, ModalContent, ModalHeader, useDisclosure } from "@nextui-org/react";
import { HelpCircle } from "react-feather";
import { useState, useEffect } from "react";
import { useBifrost } from "../../../redux/actions/bifrost/useBifrostAction.ts";
import { RosService } from "../../../ros/services/rosService.ts";
import { useSelector } from "react-redux";
import { RootState } from "../../../redux/RootState.ts";
import { IRosScienceInterfacesRamanSpecRequest } from "../../../ros/rosTypes.ts";


function checkPeriods(shPeriod: number, icgPeriod: number) {
    return ((20 <= shPeriod && shPeriod <= 4294967295 && shPeriod % 1 == 0) && 
    (14776 <= icgPeriod && icgPeriod <= 4294967295 && icgPeriod % 1 == 0 ) &&
    ((icgPeriod/shPeriod)%1 == 0))
}

function checkAverage(average: number) {
    return (1 <= average && average <= 15 && average % 1 == 0)
}

const RamanCCDInputs: React.FC = () => {
    const {isOpen, onOpen, onOpenChange} = useDisclosure();

    const [port, setPort] = useState("/dev/ttyACM0");
    const [shPeriod, setSHPeriod] = useState(200);
    const [icgPeriod, setICGPeriod] = useState(100000);
    const [average, setAverage] = useState(1);
    const [singleCollectionMode, setSingleCollectionMode] = useState(true);
    const [currentlyInContinuous, setCurrentlyInContinuous] = useState(false);

    const response = useSelector((state: RootState) => state.ramanSpecServiceStore);
    
    const bifrost = useBifrost({ service: RosService.CALL_RAMAN_SPEC });

    const sendRamanRequest = (request: IRosScienceInterfacesRamanSpecRequest) => bifrost.callService(request, { sendToRedux: true });

    useEffect(() => {
        bifrost.syncWithTopic();
        if (response.continuousendedsignal == true) {
            setCurrentlyInContinuous(false);
        }
      }, [bifrost, response.continuousendedsignal]);

    return (
        <Card className="h-40 m-1 p-5 flex flex-row flex-wrap flex-1 space-x-10">
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
                        <p className="mb-3">Average determines the amount of samples taken and averaged by the firmware, its minimum value is 1 and its maximum value is 15 (must be an integer).</p>
                    </ModalBody>
                    </>
                )}
                </ModalContent>
            </Modal>
            <Input onValueChange={(value: string) => setPort(value)} className="shrink-0 w-44 grow" type="port" label="Port" placeholder="Example: /dev/ttyACM0" defaultValue={port} />
            <Input onValueChange={(value: string) => setSHPeriod(+value)} className="shrink-0 w-36 grow" type="shperiod" label="SH Period" placeholder="[20, 4294967295]" defaultValue={shPeriod.toString()} />
            <Input onValueChange={(value: string) => setICGPeriod(+value)} className="shrink-0 w-40 grow" type="icgperiod" label="ICG Period" placeholder="[14776, 4294967295]" defaultValue={icgPeriod.toString()} />
            <Input onValueChange={(value: string) => setAverage(+value)} className="shrink-0 w-16 grow" type="average" label="Average" placeholder="[1, 15]" defaultValue={average.toString()} />
            <Button 
            onPress={() => {setSingleCollectionMode(!singleCollectionMode)}}
            color= {singleCollectionMode ? "primary" : "secondary"} className="h-14 shrink-0 w-52" radius="lg">
                Collection Mode: {singleCollectionMode ? "Single" : "Continuous"}
            </Button>
            <Button onPress={() => {
                if (checkPeriods(shPeriod, icgPeriod) && checkAverage(average)) {
                    sendRamanRequest({
                        port: port,
                        shperiod: shPeriod,
                        icgperiod: icgPeriod,
                        average: average,
                        singlecollectionmode: singleCollectionMode,
                        continuousendsignal: currentlyInContinuous
                    });
                    if (!singleCollectionMode) {
                        setCurrentlyInContinuous(true);
                    }
                } else {
                    onOpen();
                }
            }} color={currentlyInContinuous ? "danger" : "primary"} className="h-14 flex flex-row shrink-0 grow w-32" radius="lg">{currentlyInContinuous ? "Stop" : "Collect"}</Button>
        </Card>
    )
}

export default RamanCCDInputs;