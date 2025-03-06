import React, { useState } from "react";
import { NumberInput , Card, CardHeader, CardBody} from "@nextui-org/react";
import { NumberInput, Card, CardHeader, CardBody } from "@heroui/react";

const RGBInputWidget: React.FC = () => {
    const [r, setR] = useState(0);
    const [g, setG] = useState(0);
    const [b, setB] = useState(0);

    const colorPreview = `rgb(${r}, ${g}, ${b})`;

    return (
        <Card className="p-4 space-y-4">
            <CardHeader className="text-xl font-bold">RGB Input Widget</CardHeader>
            <CardBody className="space-y-3">
                <div className="flex gap-4">
                    <div className="w-1/3">
                        <label htmlFor="r" className="block text-sm font-semibold">R </label>
                        <NumberInput
                            id="r"
                            value={r}
                            onValueChange={setR}
                            minValue={0}
                            maxValue={255}
                            step={1}
                        />
                    </div>

                    <div className="w-1/3">
                        <label htmlFor="g" className="block text-sm font-semibold">G </label>
                        <NumberInput
                            id="g"
                            value={g}
                            onValueChange={setG}
                            minValue={0}
                            maxValue={255}
                            step={1}
                        />
                    </div>

                    <div className="w-1/3">
                        <label htmlFor="b" className="block text-sm font-semibold">B </label>
                        <NumberInput
                            id="b"
                            value={b}
                            onValueChange={setB}
                            minValue={0}
                            maxValue={255}
                            step={1}
                        />
                    </div>
                </div>

                {/* Color Preview Box */}
                <div className="h-24 w-full flex items-center justify-center mt-4 rounded-lg" style={{ backgroundColor: colorPreview }}>
                    <span className="text-lg font-bold p-2">
                        RGB: {colorPreview}
                    </span>
                </div>
            </CardBody>
        </Card>
    );
};

export default RGBInputWidget;
