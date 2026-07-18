/**
 * Author: Connor Macdougall
 * This component is for the mechanical inputs that would be necessary to collect raman spectra.
 * It uses the old mechanical inputs (from 2023)
 */

import { Button, Card, CardHeader, Input, Modal, ModalBody, ModalContent, ModalHeader, useDisclosure, Tabs, Tab, Avatar } from "@nextui-org/react";
import { useEffect, useState } from "react";
import { HelpCircle } from "react-feather";
import { IRosScienceInterfacesRamanMechRequest } from "../../../ros/rosTypes.ts";
import { useSelector } from "react-redux";
import { RootState } from "../../../redux/RootState.ts";
import { RosService } from "../../../ros/services/rosService.ts";
import { useBifrost } from "../../../redux/actions/bifrost/useBifrostAction.ts";
import { RosTopic } from "../../../ros/topics/rosTopic.ts";

const RamanMechanicalInputs: React.FC = () => {
    const RED_LASER_KEY = "red-laser";
    const GREEN_LASER_KEY = "green-laser";
    const OFF_LASER_KEY = "off";


    // service bifrost
    const ramanMechRequest = useSelector((state: RootState) => state.ramanMechServiceStore);
    const inputBifrost = useBifrost({ service: RosService.CALL_RAMAN_MECH });
    const sendRamanMechRequest = (request: IRosScienceInterfacesRamanMechRequest) => inputBifrost.callService(request, { sendToRedux: true });

    // topic bifrost
    const ramanMechState = useSelector((state: RootState) => state.ramanMechMessageStore);
    const bifrost = useBifrost({ topic: RosTopic.RAMAN_MECH_MSG });
    useEffect(() => { bifrost.syncWithTopic(); }, [bifrost]);


    const {isOpen, onOpen, onOpenChange} = useDisclosure();
    const [currentLaserKey, setCurrentLaserKey] = useState(OFF_LASER_KEY);
    const [ramanMechInputs, setRamanMechInputs] = useState({
        green_laser_on: false,
        red_laser_on: false,
        filter_selection: 0,
        stepper_value: 0,
        mirror_servo: 0
    });

    const checkFilterSelection = (filterValue: number) => {
        if (0 <= filterValue && filterValue % 40 == 0 && filterValue <= 160) {
            return true;
        }
        return false;
    }

    const checkStepperValue = (stepperValue: number) => {
        if (0 <= stepperValue && stepperValue % 1 == 0 && stepperValue <= 4) {
            return true;
        }
        return false;
    }

    const checkMirrorServo = (mirrorServoValue: number) => {
        if (mirrorServoValue == 0 || mirrorServoValue == 90) {
            return true;
        }
        return false;
    }


    return (
        <Card className="m-1 p-5 flex flex-row flex-wrap flex-1 space-x-10 justify-around h-60">
            <CardHeader className="shrink-0 w-40 p-1">Mechanical Inputs</CardHeader>
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
                <Avatar size="lg" name={ramanMechState.green_laser_on ? "On" : ramanMechState.red_laser_on ? "On" : "Off"} className={ramanMechState.green_laser_on ? "bg-green-600" : ramanMechState.red_laser_on ? "bg-red-600" : ""}></Avatar>
                <Tabs className="mt-1" size="lg"
                    selectedKey={currentLaserKey}
                    onSelectionChange={(key) => {
                        setCurrentLaserKey(key.toString());
                        if (key == GREEN_LASER_KEY) {
                            setRamanMechInputs({...ramanMechInputs, green_laser_on: true, red_laser_on: false});
                            sendRamanMechRequest({...ramanMechInputs, green_laser_on: true, red_laser_on: false});
                        } else if (key == RED_LASER_KEY) {
                            setRamanMechInputs({...ramanMechInputs, green_laser_on: false, red_laser_on: true});
                            sendRamanMechRequest({...ramanMechInputs, green_laser_on: false, red_laser_on: true});
                        } else {
                            setRamanMechInputs({...ramanMechInputs, green_laser_on: false, red_laser_on: false});
                            sendRamanMechRequest({...ramanMechInputs, green_laser_on: false, red_laser_on: false});
                        }
                    }}
                >
                    <Tab key="green-laser" className="text-green-400" title="Green" />
                    <Tab key="red-laser" title="Red" />
                    <Tab key="off" title="Off" />
                </Tabs>
            </div>
            <div className="mx-1 flex w-52 flex-row">
                <Avatar size="lg" name={ramanMechState.filter_selection.toString()}></Avatar>
                <Input onValueChange={(value: string) => { if (value != "" && checkFilterSelection(+value)) {
                    setRamanMechInputs({...ramanMechInputs, filter_selection: +value});
                }}} className="shrink-0 w-20 grow" type="filterselection" label="Filter Selection" placeholder="{0, 40, 80, 120, 160}" defaultValue={ramanMechState.filter_selection.toString()} />
            </div>
            <div className="mx-1 flex w-44 flex-row">
                <Avatar size="lg" name={ramanMechState.stepper_value.toString()}></Avatar>
                <Input onValueChange={(value: string) => { if (value != "" && checkStepperValue(+value)) {
                    setRamanMechInputs({...ramanMechInputs, stepper_value: +value});
                }}} className="shrink-0 w-20 grow" type="steppervalue" label="Stepper Value" placeholder="{0, 1, 2, 3, 4}" defaultValue={ramanMechState.stepper_value.toString()} />
            </div>
            <div className="mx-1 flex w-40 flex-row">
                <Avatar size="lg" name={ramanMechState.mirror_servo.toString()}></Avatar>
                <Input onValueChange={(value: string) => { if (value != "" && checkMirrorServo(+value)) {
                    setRamanMechInputs({...ramanMechInputs, mirror_servo: +value});
                }}} className="shrink-0 w-20 grow" type="mirrorservo" label="Mirror Servo" placeholder="{0, 90}" defaultValue={ramanMechState.mirror_servo.toString()} />
            </div>
            <Button onPress={() => {
                sendRamanMechRequest(ramanMechInputs);
            }} color={ramanMechRequest.success ? "primary" : "danger"} className="h-14 flex flex-row shrink-0 w-60" radius="lg">{ramanMechRequest.success ? "Refresh" : "Error: Refresh"}</Button>
        </Card>
    )
}

export default RamanMechanicalInputs;