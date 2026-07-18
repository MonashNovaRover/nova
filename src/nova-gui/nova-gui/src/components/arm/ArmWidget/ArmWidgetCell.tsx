import {
    Card,
    CardBody,
    CardProps
  } from "@nextui-org/react";
  import React, {ReactNode} from "react";
  import { OverlayedProgress } from "../../shared/components/OverlayedProgress/OverlayedProgress.tsx";
  
  export interface IArmWidgetCellProps extends CardProps {
    jointCurrent: number,
    jointVelocity: number,
    progressMaxVelocity: number,
    label: ReactNode
  }

    const ArmWidgetCell: React.FC<IArmWidgetCellProps> = (props: IArmWidgetCellProps) => {
        const jointCurrent = (
        <OverlayedProgress size="lg"
                             value={props.jointCurrent}
                             maxValue={1}
                             aria-label="Joint Current"
                             autoColor={true}
                             disableAnimation={false}>
            <div className="grid grid-flow-col gap-3 auto-cols-fr text-small">
            <span>{`${(props.jointCurrent * 100).toFixed(0)}%`}</span>
            </div>
        </OverlayedProgress>
        );

        const jointVelocity = (
            <OverlayedProgress size="lg"
                               value={props.jointVelocity}
                               maxValue={props.progressMaxVelocity}
                               aria-label="Joint Velocity"
                               autoColor={true}
                               disableAnimation={false}>
                <div className="grid grid-flow-col gap-3 auto-cols-fr text-small">
                    <span>{`${props.jointVelocity.toFixed(2)} rad/s`}</span>
                </div>
            </OverlayedProgress>
        );
    
        const label = (
        <span className="text-sm uppercase tracking-widest text-center text-default-900 text-opacity-80">
            {props.label}
        </span>
        )
    
        return <Card shadow="sm" {...props} >
        <CardBody className="pt-1 pl-1.5 pr-1.5 pb-1.5 flex gap-1 font-semibold flex-col content-center bg-content2">
            {label}
            <div className="flex flex-col gap-2 content-center">
            {jointCurrent}
            {jointVelocity}
            </div>
        </CardBody>
        </Card>
    }

    export  default ArmWidgetCell;