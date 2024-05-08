/**
 * Author: Connor Macdougall
 * This component is for the mechanical inputs that would be necessary to collect raman spectra.
 * It uses the old mechanical inputs (from 2023)
 */

import { Button, Card, CardHeader, Input, Modal, ModalBody, ModalContent, ModalHeader, useDisclosure, Tabs, Tab, Avatar } from "@nextui-org/react";
import { useState } from "react";
import { HelpCircle } from "react-feather";

const RamanMechanicalInputs: React.FC = () => {
    const RED_LASER_KEY = "red-laser";
    const GREEN_LASER_KEY = "green-laser";
    const OFF_LASER_KEY = "off";
    const ON_PUMP_KEY = "on";
    const OFF_PUMP_KEY = "off";


    const {isOpen, onOpen, onOpenChange} = useDisclosure();
    const [currentLaserKey, setCurrentLaserKey] = useState(OFF_LASER_KEY);
    const [currentPumpKey, setCurrentPumpKey] = useState(OFF_PUMP_KEY);
    const [ramanMechInputs, setRamanMechInputs] = useState({
        greenLaserOn: false,
        redLaserOn: false,
        pumpOn: false,
        filterSelection: 0,
        stepperValue: 0,
        mirrorServo: 0
    });

    const ramanMechState = {
        greenLaserOn: false,
        redLaserOn: false,
        pumpOn: false,
        filterSelection: 0,
        stepperValue: 0,
        mirrorServo: 0
    } // change this to bifrost store when linked

    const sendRamanMechRequest = () => {console.log(ramanMechInputs)}

    const checkFilterSelection = (filterValue: number) => {
        if (!(filterValue == undefined)) {
            return true;
        }
        return false;
    }

    const checkStepperValue = (stepperValue: number) => {
        if (!(stepperValue == undefined)) {
            return true;
        }
        return false;
    }

    const checkMirrorServo = (mirrorServoValue: number) => {
        if (!(mirrorServoValue == undefined)) {
            return true;
        }
        return false;
    }


    return (
        <Card className="m-1 p-2 flex flex-row flex-1 space-x-2">
            <CardHeader className="shrink-0 w-24 p-1">Mechanical Inputs</CardHeader>
            <Button isIconOnly className="w-8 h-8 m-3" radius="md" onPress={onOpen}>
                <HelpCircle className="w-6 h-6" />
            </Button>
            <Modal className="dark text-foreground" isOpen={isOpen} onOpenChange={onOpenChange} isDismissable={false}>
                <ModalContent>
                {() => (
                    <>
                    <ModalHeader className="flex flex-col gap-1">Mechanical Inputs Help</ModalHeader>
                    <ModalBody>
                        <p className="mb-3">Oh.</p>
                    </ModalBody>
                    </>
                )}
                </ModalContent>
            </Modal>
            <div className="mx-1 w-60 flex flex-row">
                <Avatar size="lg" name={"Off"}></Avatar>
                <Tabs className="mt-1" size="lg"
                    selectedKey={currentLaserKey}
                    onSelectionChange={(key) => {
                        setCurrentLaserKey(key.toString())
                        if (key == GREEN_LASER_KEY) {
                            setRamanMechInputs({...ramanMechInputs, greenLaserOn: true, redLaserOn: false})
                        } else if (key == RED_LASER_KEY) {
                            setRamanMechInputs({...ramanMechInputs, greenLaserOn: false, redLaserOn: true})
                        } else {
                            setRamanMechInputs({...ramanMechInputs, greenLaserOn: false, redLaserOn: false})
                        }
                    }}
                >
                    <Tab key="green-laser" className="text-green-400" title="Green" />
                    <Tab key="red-laser" title="Red" />
                    <Tab key="off" title="Off" />
                </Tabs>
            </div>
            <div className="mx-1 flex flex-row">
                <Avatar size="lg" name={"Off"}></Avatar>
                <Tabs className="mt-1" size="lg"
                    selectedKey={currentPumpKey}
                    onSelectionChange={(key) => {
                        setCurrentPumpKey(key.toString())
                        if (key == ON_PUMP_KEY) {
                            setRamanMechInputs({...ramanMechInputs, pumpOn: true})
                        } else {
                            setRamanMechInputs({...ramanMechInputs, pumpOn: false})
                        }
                    }}
                >
                    <Tab key="on" title="Pump On"/>
                    <Tab key="off" title="Pump Off"/>
                </Tabs>
            </div>
            <div className="mx-1 flex w-40 flex-row">
                <Avatar size="lg" name={"Off"}></Avatar>
                <Input onValueChange={(value: string) => { if (checkFilterSelection(+value)) {
                    setRamanMechInputs({...ramanMechInputs, filterSelection: +value})
                }}} className="shrink-0 w-20 grow" type="filterselection" label="Filter Selection" placeholder="[min, max]" defaultValue={ramanMechState.filterSelection.toString()} />
            </div>
            <div className="mx-1 flex flex-row">
                <Avatar size="lg" name={"Off"}></Avatar>
                <Input onValueChange={(value: string) => { if (checkStepperValue(+value)) {
                    setRamanMechInputs({...ramanMechInputs, stepperValue: +value});
                }}} className="shrink-0 w-20 grow" type="steppervalue" label="Stepper Value" placeholder="[min, max]" defaultValue={ramanMechState.stepperValue.toString()} />
            </div>
            <div className="mx-1 flex flex-row">
                <Avatar size="lg" name={"Off"}></Avatar>
                <Input onValueChange={(value: string) => { if (checkMirrorServo(+value)) {
                    setRamanMechInputs({...ramanMechInputs, mirrorServo: +value});
                }}} className="shrink-0 w-20 grow" type="mirrorservo" label="Mirror Servo" placeholder="[min, max]" defaultValue={ramanMechState.mirrorServo.toString()} />
            </div>
            <Button onPress={() => {
                sendRamanMechRequest();
            }} color="primary" className="h-14 flex flex-row shrink-0 grow w-20" radius="lg">Update</Button>
        </Card>
    )
}

export default RamanMechanicalInputs;