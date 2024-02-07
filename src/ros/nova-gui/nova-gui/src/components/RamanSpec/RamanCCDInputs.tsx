/**
 * Author: Connor Macdougall
 * This component is for the CCD inputs.
 * It sends requests to the 'raman_spectra' ROS service.
 */

import { Button, Card, CardHeader, Input, Modal, ModalBody, ModalContent, ModalHeader, useDisclosure } from "@nextui-org/react";
import { HelpCircle } from "react-feather";
import { useState } from "react";
import { useBifrost } from "../../redux/actions/bifrost/useBifrostAction";
import { RosService } from "../../ros/services/rosService";
import { IRosCoreRamanSpecRequest } from "../../ros/rosTypes";

function checkPeriods(shPeriod: number, icgPeriod: number) {
    return ((20 <= shPeriod && shPeriod <= 4294967295 && shPeriod % 1 == 0) && 
    (14776 <= icgPeriod && icgPeriod <= 4294967295 && icgPeriod % 1 == 0 ) &&
    ((icgPeriod/shPeriod)%1 == 0))
}

function checkAverage(average: number) {
    return (1 <= average && average <= 15 && average % 1 == 0)
}

function checkResolution(resolution: number) {
    return (1 <= resolution && resolution <= 255 && resolution % 1 == 0)
}

const RamanCCDInputs: React.FC = () => {
    const {isOpen, onOpen, onOpenChange} = useDisclosure();
    
    const bifrost = useBifrost({ service: RosService.CALL_RAMAN_SPEC });

    const sendRamanRequest = (request: IRosCoreRamanSpecRequest) => bifrost.callServiceToRedux(request);

    const [port, setPort] = useState("/dev/ttyACM0");
    const [shPeriod, setSHPeriod] = useState(200);
    const [icgPeriod, setICGPeriod] = useState(100000);
    const [average, setAverage] = useState(1);
    const [singleCollectionMode, setSingleCollectionMode] = useState(true);
    const [resolutionReductionFactor, setResolutionReductionFactor] = useState(1);

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
                        <p>Average determines the amount of samples taken and averaged by the firmware, its minimum value is 1 and its maximum value is 15 (must be an integer).</p>
                        <p className="mb-2.5">Resolution Reduction Factor determines how detailed the output is. For example, if it is 100, then for every 100 points outputted by the CCD, 1 point (averaging those 100 points) is received. Its minimum value is 1 and its maximum value is 255 (must be an integer).</p>
                    </ModalBody>
                    </>
                )}
                </ModalContent>
            </Modal>
            <Input onValueChange={(value: string) => setPort(value)} className="shrink-0 w-44 grow" type="port" label="Port" placeholder="Example: /dev/ttyACM0" defaultValue={port} />
            <Input onValueChange={(value: string) => setSHPeriod(+value)} className="shrink-0 w-36 grow" type="shperiod" label="SH Period" placeholder="[20, 4294967295]" defaultValue={shPeriod.toString()} />
            <Input onValueChange={(value: string) => setICGPeriod(+value)} className="shrink-0 w-40 grow" type="icgperiod" label="ICG Period" placeholder="[14776, 4294967295]" defaultValue={icgPeriod.toString()} />
            <Input onValueChange={(value: string) => setAverage(+value)} className="shrink-0 w-16 grow" type="average" label="Average" placeholder="[1, 15]" defaultValue={average.toString()} />
            <Input onValueChange={(value: string) => setResolutionReductionFactor(+value)} className="shrink-0 w-24 grow" type="resolutionreductionfactor" label="Resolution Reduction Factor" placeholder="[1, 255]" defaultValue={resolutionReductionFactor.toString()} />
            <Button 
            onPress={() => {setSingleCollectionMode(!singleCollectionMode)}}
            color= {singleCollectionMode ? "primary" : "secondary"} className="h-14 shrink-0 w-52" radius="lg">
                Collection Mode: {singleCollectionMode ? "Single" : "Continuous"}
            </Button>
            <Button onPress={() => {
                if (checkPeriods(shPeriod, icgPeriod) && checkAverage(average) && checkResolution(resolutionReductionFactor)) {
                    sendRamanRequest({
                        port: port,
                        shperiod: shPeriod,
                        icgperiod: icgPeriod,
                        average: average,
                        resolutionreductionfactor: resolutionReductionFactor,
                        singlecollectionmode: singleCollectionMode,
                        continuousendsignal: false
                    })
                } else {
                    onOpen();
                }
            }} color="primary" className="h-14 flex flex-row shrink-0 grow w-32" radius="lg">Collect</Button>
        </Card>
    )
}

export default RamanCCDInputs;