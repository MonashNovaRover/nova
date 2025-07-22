import {
    Card,
    CardBody,
    CardProps
  } from "@nextui-org/react";
  import React, {ReactNode} from "react";
  import { OverlayedProgress } from "../../shared/components/OverlayedProgress/OverlayedProgress.tsx";
  
  export interface IArmWidgetCellProps extends CardProps {
    jointValue: number,
    label: ReactNode
  }

    const ArmWidgetCell: React.FC<IArmWidgetCellProps> = (props: IArmWidgetCellProps) => {
        const jointProgress = (
        <OverlayedProgress size="lg"
                             value={props.jointValue}
                             maxValue={1}
                             aria-label="Joint Amount"
                             autoColor={true}
                             disableAnimation={false}>
            <div className="grid grid-flow-col gap-3 auto-cols-fr text-small">
            <span>JOINT</span>
            <span>{`${(props.jointValue * 100).toFixed(0)}%`}</span>
            </div>
        </OverlayedProgress>
        );
    
        const label = (
        <span className="text-sm uppercase tracking-widest text-center text-default-900 text-opacity-80">
            {props.label}
        </span>
        )
    
        return <Card shadow="sm" {...props} >
        <CardBody className="pt-1 flex gap-1 font-semibold flex-col content-center bg-content2">
            {label}
            <div className="flex flex-col gap-2 content-center">
            {jointProgress}
            </div>
        </CardBody>
        </Card>
    }

    export  default ArmWidgetCell;