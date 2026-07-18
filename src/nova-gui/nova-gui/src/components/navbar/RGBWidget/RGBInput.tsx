import React, {useCallback} from "react";
import {Button, Card, CardProps, Input, Tooltip} from "@nextui-org/react";
import {SubCardLabel} from "../../shared/components/Labels.tsx";
import {useBifrost} from "../../../redux/actions/bifrost/useBifrostAction.ts";
import {RosService} from "../../../ros/services/rosService.ts";
import {useGenericStore} from "../../../hooks/useGenericStore.ts";

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
    const [tempR, tempG, tempB] = [rgbValues.r, rgbValues.b, rgbValues.g]

    const serviceBifrost = useBifrost({service: RosService.RGBInput});

    const sendRGBValues = useCallback((flash: boolean = false) => {
        try{

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
    }, [tempR, tempG, tempB, serviceBifrost, setRgbValues]);

    const handleRChange = useCallback((value: string) => {
        const numValue = Number(value);
        if (!isNaN(numValue) && numValue >= 0 && numValue <= 255) setRgbValues({...rgbValues, r: value})
    }, [rgbValues, setRgbValues]);

    const handleGChange = useCallback((value: string) => {
        const numValue = Number(value);
        if (!isNaN(numValue) && numValue >= 0 && numValue <= 255) setRgbValues({...rgbValues, g: value})
    }, [rgbValues, setRgbValues]);

    const handleBChange = useCallback((value: string) => {
        const numValue = Number(value);
        if (!isNaN(numValue) && numValue >= 0 && numValue <= 255) setRgbValues({...rgbValues, b: value})
    }, [rgbValues, setRgbValues]);

    const flashGreen = () => {
        serviceBifrost.callService(
          { r: 0, g: 255, b: 0, flash: true },
          { noErrorToast: false, responseToast: true }
        );
    };

    const colorPreview = `rgb(${tempR || 0}, ${tempG || 0}, ${tempB || 0})`;
    const previewTextColor = ((+tempR*299) + (+tempG*587) + (+tempB * 114))/1000 > 150 ? "black" : "white" // determine luminesence and adjust text colour accordingly

    return (
        <Card {...props} className="space-y-3 p-3">
            <Card className="space-y-3 p-3 bg-content2" shadow="sm">
                <div className="flex grid-cols-3 gap-5">
                    <div className="w-1/3">
                        <label htmlFor="r" className="block text-sm font-semibold pl-3">R</label>
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
                        <label htmlFor="g" className="block text-sm font-semibold pl-3">G</label>
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
                        <label htmlFor="b" className="block text-sm font-semibold pl-3">B</label>
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
                <div className={`h-24 w-full flex items-center justify-center mt-4 rounded-lg bg-${colorPreview}`} style={{ backgroundColor: colorPreview}}>
                    <span className={`text-lg font-bold p-2 text-${previewTextColor}`}>
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
                          className="w-6 h-6 bg-[rgb(255,0,0)]"
                          aria-label="Red"
                          onPress={()=>setRgbValues({r: "255", g: "0", b: "0"})}
                        />
                    </Tooltip>
                    <Tooltip content="Green" placement="top">
                        <Button
                          isIconOnly
                          size="sm"
                          className="w-6 h-6 bg-[rgb(0,255,0)]"
                          aria-label="Green"
                          onPress={()=>setRgbValues({r: "0", g: "255", b: "0"})}
                        />
                    </Tooltip>
                    <Tooltip content="Blue" placement="top">
                        <Button
                          isIconOnly
                          size="sm"
                          className="w-6 h-6 bg-[rgb(0,0,255)]"
                          aria-label="Blue"
                          onPress={()=>setRgbValues({r: "255", g: "0", b: "255"})}
                        />
                    </Tooltip>
                    <Tooltip content="Yellow" placement="top">
                        <Button
                          isIconOnly
                          size="sm"
                          className="w-6 h-6 bg-[rgb(255,255,0)]"
                          aria-label="Yellow"
                          onPress={()=>setRgbValues({r: "255", g: "255", b: "0"})}
                        />
                    </Tooltip>
                    <Tooltip content="Pink" placement="top">
                        <Button
                          isIconOnly
                          size="sm"
                          className="w-6 h-6 bg-[rgb(255,105,180)]"
                          aria-label="Pink"
                          onPress={()=>setRgbValues({r: "255", g: "105", b: "180"})}
                        />
                    </Tooltip>
                        <Button
                          size="sm"
                          className="text-xs px-2 bg-[rgb(0,64,0)] text-[rgb(127,255,127)]"
                          variant="flat"
                          onPress={flashGreen}
                        >
                        Flash Green
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