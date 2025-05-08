import React, {useCallback, useState} from "react";
import {Button, Card, CardProps, Input} from "@nextui-org/react";
import {SubCardLabel} from "../shared/Labels";
import {useBifrost} from "../../redux/actions/bifrost/useBifrostAction.ts";
import {RosService} from "../../ros/services/rosService.ts";
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
    // const [r, setR] = useState("0");
    // const [g, setG] = useState("0");
    // const [b, setB] = useState("0");
    const [rgbValues, setRgbValues] = useGenericStore<{
        r: string;
        g: string;
        b: string;
    }>("rgbLedStore");

    const {r,g,b} = rgbValues;

    const serviceBifrost = useBifrost({service: RosService.RGBInput});

    const sendRGBValues = useCallback(() => {
        try{
            const rValue = Number(r);
            const gValue = Number(g);
            const bValue = Number(b);

            if (isNaN(rValue) || isNaN(gValue) || isNaN(bValue) || rValue < 0 || gValue < 0 || bValue < 0 || rValue > 255 || gValue > 255 || bValue > 255) {
                console.error("Invalid input for RGB values");
                return;
            }

            serviceBifrost.callService(
                {r:rValue, g:gValue, b:bValue},
                {noErrorToast: false, responseToast:true},
            );
        }catch (e) {
            console.error("Could not send RGB Values:",e)
        }
    },[r,g,b,serviceBifrost])

    const handleRChange = useCallback((value: string) => {
        const numValue = Number(value);
        if (!isNaN(numValue) && numValue >= 0 && numValue <= 255) {
            setRgbValues({...rgbValues, r:value}) ;
        }
    }, [rgbValues, setRgbValues]);

    const handleGChange = useCallback((value: string) => {
        const numValue = Number(value);
        if (!isNaN(numValue) && numValue >= 0 && numValue <= 255){
            setRgbValues({...rgbValues, g:value}) ;
        }
    }, [rgbValues, setRgbValues]);

    const handleBChange = useCallback((value: string) => {
        const numValue = Number(value);
        if (!isNaN(numValue) && numValue >= 0 && numValue <= 255){
            setRgbValues({...rgbValues, b:value}) ;
        }
    }, [rgbValues, setRgbValues]);

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
                            value={r}
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
                            value={g}
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
                            value={b}
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
            <Button onClick={sendRGBValues} color="primary">
                Set LEDs
            </Button>
        </Card>
    );
};

export default RGBInputWidget;