/**
 * Author: Connor Macdougall
 * This component is for the CCD inputs.
 * It sends requests to the 'raman_spectra' ROS service.
 */

import { Button, Card, CardHeader, Input, Modal, ModalBody, ModalContent, ModalHeader, useDisclosure } from "@nextui-org/react";
import { HelpCircle } from "react-feather";
import { useState, useEffect } from "react";
import { useBifrost } from "../../redux/actions/bifrost/useBifrostAction";
import { RosService } from "../../ros/services/rosService";
import { IRosNovaInterfacesRamanSpecRequest } from "../../ros/rosTypes";
import { useSelector } from "react-redux";
import { RootState } from "../../redux/RootState";


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

    const sendRamanRequest = (request: IRosNovaInterfacesRamanSpecRequest) => bifrost.callServiceToRedux(request);

    useEffect(() => {
        bifrost.syncWithTopic();
        if (response.continuousendedsignal == true) {
            setCurrentlyInContinuous(false);
        }
      }, [bifrost]);

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
                    } /*
                    let ros = new ROSLIB.Ros({
                        url: 'ws://localhost:9090'	                            
                    });
                    ros.on('error', () => {console.log("error")});	                        
                    ros.on('connection', () => {console.log("connected")});	                        
                    ros.on('close', () => {console.log("closed")});	                        
                    let ramanSpectra = new ROSLIB.Topic({	             
                        ros: ros,	             
                        name: 'science/raman_spec_msg',	               
                        messageType: "core/msg/RamanSpectrum"	      
                    });	              
                    let fakespectra = [average, 11, 9, 8, 9, 10, 12, 11, 9, 11, 10, 11, 10, 9, 11, 12, 13, 15, 17, 20, 23, 24, 28, 33, 39, 47, 58, 70, 66, 54, 50, 70, 90, 65, 40, 35, 34, 34, 35, 35, 34, 34, 33, 32, 31, 31, 30, 31, 32]	                       
                    let spectra1 = new ROSLIB.Message({	                                
                        isvalid: true,	                                    
                        spectrum: fakespectra	                                    
                    });	                            
                    ramanSpectra.publish(spectra1);
                    setTimeout(() => {fakespectra = [15, 11, 9, 8, 9, 15, 12, 11, 9, 11, 10, 11, 10, 9, 11, 12, 13, 15, 17, 20, 23, 24, 28, 33, 39, 65, 58, 70, 66, 54, 50, 70, 90, 65, 40, 35, 34, 34, 35, 35, 34, 34, 33, 32, 31, 31, 30, 31, 32]	                       
                        let spectra2 = new ROSLIB.Message({	                                
                            isvalid: true,	                                    
                            spectrum: fakespectra	                                    
                        });	                            
                        ramanSpectra.publish(spectra2);}, 1000);
                    */
                } else {
                    onOpen();
                }
            }} color={currentlyInContinuous ? "danger" : "primary"} className="h-14 flex flex-row shrink-0 grow w-32" radius="lg">{currentlyInContinuous ? "Stop" : "Collect"}</Button>
        </Card>
    )
}

export default RamanCCDInputs;