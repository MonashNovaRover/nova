// import {
//     Card,
//     CardBody,
//     CardProps
//   } from "@nextui-org/react";
// import React, {ReactNode} from "react";
// import './ArmWidget.css';

// // Properties for the ARM component.
// export interface IArmWidgetCellProps extends CardProps {
//   label: ReactNode,
//   jointValue: number
// }

// /**
//  * A component for displaying telemetry for a single joint
//  */

// const ArmWidgetCell: React.FC<IArmWidgetCellProps> = (props: IArmWidgetCellProps) => {

//     const jointProgress = (
//         <div className="grid grid-flow-col gap-3 auto-cols-fr text-small">
//             <span>{`${(props.jointValue * 100).toFixed(0)}%`}</span>
//         </div>
//     );