import React, {useCallback, useEffect, useState} from "react";
import {Card, CardHeader, CardProps, Input} from "@nextui-org/react";
import {SubCardLabel} from "../shared/Labels";
import {useBifrost} from "../../redux/actions/bifrost/useBifrostAction.ts";
import {RosService} from "../../ros/services/rosService.ts";
import { useSelector } from "react-redux";
import { RootState } from "../../redux/RootState";
import {useGenericStore} from "../../hooks/useGenericStore.ts";

interface RGBInputWidgetProps extends CardProps {}
/**
 * Widget for changing the colors of the rover's LEDs through RGB input
 * @param R
 * @param G
 * @param B
 * @constructor
 */
const RGBInputWidget: React.FC<RGBInputWidgetProps> = (props) => {
    const [rgbValues, setRgbValues] = useGenericStore<{
        r: string;
        g: string;
        b: string;
    }>("rgbLedStore");

    const [tempR, setTempR] = useState(rgbValues.r);
    const [tempG, setTempG] = useState(rgbValues.g);
    const [tempB, setTempB] = useState(rgbValues.b);

    const {r,g,b} = rgbValues;

    const serviceBifrost = useBifrost({service: RosService.RGBInput});

    useEffect(() => {
        setTempR(rgbValues.r);
        setTempG(rgbValues.g);
        setTempB(rgbValues.b);
    }, [r,g,b]);

    const sendRGBValues = useCallback((flash = false) => {
        try{
            // const rValue = Number(r);
            // const gValue = Number(g);
            // const bValue = Number(b);

            const rValue = Number(tempR);
            const gValue = Number(tempG);
            const bValue = Number(tempB);

            if (isNaN(rValue) || isNaN(gValue) || isNaN(bValue) || rValue < 0 || gValue < 0 || bValue < 0 || rValue > 255 || gValue > 255 || bValue > 255) {
                console.error("Invalid input for RGB values");
                return;
            }

            serviceBifrost.callService(
                {r:rValue, g:gValue, b:bValue, flash},
                {noErrorToast: false, responseToast:true},
            );

            if (!flash) { //only updates generic store values when flash is false
                setRgbValues({r: tempR, g: tempG, b: tempB});
            }

        }catch (e) {
            console.error("Could not send RGB Values:",e)
        }
    },[r,g,b,serviceBifrost])

    const serviceBifrost = useBifrost({service: RosService.RGBInput});

    const rgbServiceResponse = useSelector(
        (state: RootState) => state.RGBInputStore
    )

    const sendRGBValues = useCallback(() => {
        try{
            const rValue = Number(r);
            const gValue = Number(g);
            const bValue = Number(b);

            if (isNaN(rValue) || isNaN(gValue) || isNaN(bValue) || rValue < 0 || gValue < 0 || bValue < 0 || rValue > 255 || gValue > 255 || bValue > 255) {
                console.error("Invalid input for RGB values");
                return;
            }

            serviceBifrost.callServiceToRedux(
                {r:rValue, g:gValue, b:bValue},
                {noErrorToast: false, responseToast:true},
            );
        }catch (e) {
            console.error("Could not send RGB Values:",e)
        }
    },[r,g,b,serviceBifrost])

    const handleRChange = useCallback((value: string) => {
        const numValue = Number(value);
        if (!isNaN(numValue) && numValue >= 0 && numValue <= 255) setTempR(value)
    }, [rgbValues, setRgbValues]);

    const handleGChange = useCallback((value: string) => {
        const numValue = Number(value);
        if (!isNaN(numValue) && numValue >= 0 && numValue <= 255) setTempG(value)
    }, [rgbValues, setRgbValues]);

    const handleBChange = useCallback((value: string) => {
        const numValue = Number(value);
        if (!isNaN(numValue) && numValue >= 0 && numValue <= 255) setTempB(value)
    }, [rgbValues, setRgbValues]);

    useEffect(() => {
        sendRGBValues();
    }, [r,g,b,sendRGBValues]);

    const colorPreview = `rgb(${r || 0}, ${g || 0}, ${b || 0})`;

    return (
        <Card {...props} className="space-y-3 p-3">
            <Card className="space-y-3 p-3 bg-content2" shadow="sm">
                <div className="flex gap-5">
                    <div className="w-1/3">
                        <label htmlFor="r" className="block text-sm font-semibold">R</label>
                        <Input
                            id="r"
                            type="number"
                            value={tempR}
                            onValueChange={handleRChange}
                            placeholder="0-255"
                            minLength={1}
                            maxLength={3}
                        />
                    </div>

                    <div className="w-1/3">
                        <label htmlFor="g" className="block text-sm font-semibold">G</label>
                        <Input
                            id="g"
                            type="number"
                            value={tempG}
                            onValueChange={handleGChange}
                            placeholder="0-255"
                            minLength={1}
                            maxLength={3}
                        />
                    </div>

                    <div className="w-1/3">
                        <label htmlFor="b" className="block text-sm font-semibold">B</label>
                        <Input
                            id="b"
                            type="number"
                            value={tempB}
                            onValueChange={handleBChange}
                            placeholder="0-255"
                            minLength={1}
                            maxLength={3}
                        />
                    </div>
                </div>
            </Card>
            <Card className="space-y-3 p-3 bg-content2" shadow="sm">
                <SubCardLabel>COLOR PREVIEW</SubCardLabel>
                <div className="h-24 w-full flex items-center justify-center mt-4 rounded-lg" style={{ backgroundColor: colorPreview }}>
                    <span className="text-lg font-bold p-2 text-white">
                        {colorPreview}
                    </span>
                </div>
            </Card>
            <Card className="p-3 bg-content2" shadow="sm">
                <SubCardLabel>PRESETS</SubCardLabel>
                <div className="flex flex-row flex-wrap gap-2 items-center justify-between">
                    <Tooltip content="Red" placement="top">
                        <Button
                          isIconOnly
                          size="sm"
                          className="w-6 h-6 bg-danger"
                          aria-label="Red"
                          onPress={() => {setTempR("255"); setTempG("0"); setTempB("0"); }}
                        />
                    </Tooltip>
                    <Tooltip content="Green" placement="top">
                        <Button
                          isIconOnly
                          size="sm"
                          className="w-6 h-6 bg-success"
                          aria-label="Green"
                          onPress={() => {setTempR("0"); setTempG("255"); setTempB("0"); }}
                        />
                    </Tooltip>
                    <Tooltip content="Blue" placement="top">
                        <Button
                          isIconOnly
                          size="sm"
                          className="w-6 h-6 bg-primary"
                          aria-label="Blue"
                          onPress={() => {setTempR("0"); setTempG("0"); setTempB("255"); }}
                        />
                    </Tooltip>
                    <Tooltip content="Yellow" placement="top">
                        <Button
                          isIconOnly
                          size="sm"
                          className="w-6 h-6 bg-warning"
                          aria-label="Yellow"
                          onPress={() => {setTempR("255"); setTempG("255"); setTempB("0"); }}
                        />
                    </Tooltip>
                    <Tooltip content="Pink" placement="top">
                        <Button
                          isIconOnly
                          size="sm"
                          className="bg-[#ff69b4] w-6 h-6"
                          aria-label="Pink"
                          onPress={() =>
                          {setTempR("255"); setTempG("105"); setTempB("180"); }
                          }
                        />
                    </Tooltip>
                        <Button
                          size="sm"
                          className="text-xs px-2"
                          variant="flat"
                          onPress={() => sendRGBValues(true)}
                        >
                        Flash LEDs
                    </Button>
                </div>
            </Card>
            <Button onPress={() => sendRGBValues(false)} color="primary">
                Set LEDs
            </Button>
        </Card>
    );
};

export default RGBInputWidget;