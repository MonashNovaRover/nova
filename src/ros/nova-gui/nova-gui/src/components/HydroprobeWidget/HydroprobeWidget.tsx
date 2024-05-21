import React, { useEffect } from "react";
import { Card, CardHeader, CardBody, CardProps } from "@nextui-org/react";
import { OverlayedProgress } from "../OverlayedProgress/OverlayedProgress.tsx";
import { useBifrost } from "../../redux/actions/bifrost/useBifrostAction";
import { RootState } from "../../redux/RootState";
import { useSelector } from "react-redux";
import { RosTopic } from "../../ros/topics/rosTopic";

export interface IHydroprobeProps extends CardProps {}

const HydroprobeWidget: React.FC<IHydroprobeProps> = (
    props: IHydroprobeProps
) => {
    const bifrost = useBifrost({ topic: RosTopic.HYDRAPROBE_DATA });
    const temperature = useSelector((state: RootState) => state.hydraprobeData.temperature);
    const moisture = useSelector((state: RootState) => state.hydraprobeData.moisture);
    const conductivity = useSelector((state: RootState) => state.hydraprobeData.conductivity);
    const dielectric = useSelector((state: RootState) => state.hydraprobeData.dielectric);

    useEffect(() => {
        bifrost.syncWithTopic();
        console.log(temperature);
    }, [bifrost]);

    const HydroprobeCardBody = (
        <CardBody className="grid grid-cols-1 sm:grid-cols-4 gap-4">
            <div className="text-center">
                <OverlayedProgress size="lg" label="Temperature" value={temperature}>
                    {temperature.toFixed(2)} °C
                </OverlayedProgress>
            </div>
            <div className="text-center">
                <OverlayedProgress size="lg" label="Moisture" value={moisture}>
                    {moisture.toFixed(2)} %
                </OverlayedProgress>
            </div>
            <div className="text-center">
                <OverlayedProgress size="lg" label="Conductivity" value={conductivity}>
                    {conductivity.toFixed(2)} mS/cm
                </OverlayedProgress>
            </div>
            <div className="text-center">
                <OverlayedProgress size="lg" label="Dielectric" value={dielectric}>
                    {dielectric.toFixed(0)}
                </OverlayedProgress>
            </div>
        </CardBody>
    );

    return (
        <Card {...props}>
            <CardHeader className="text-h1 pb-0">Hydroprobe Data</CardHeader>
            {HydroprobeCardBody}
        </Card>
    );
};

export default HydroprobeWidget;
