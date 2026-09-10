import { ProgressBar, ProgressBarProps } from "@heroui/react";
import type { ReactNode } from "react";
import "./OverlayedProgress.css";

// Properties for the DriveModeButton component.
export interface IDriveOverlayedProps extends Omit<ProgressBarProps, "children"> {
    // Set to true when you want the color of the bar to change to red for values over 75%
  autoColor?: boolean;
  label?: ReactNode;
  valueLabel?: ReactNode;
  radius?: string;
  children?: ReactNode;
}

export const OverlayedProgress: React.FC<IDriveOverlayedProps> = (props: IDriveOverlayedProps) =>
{
  const {autoColor, children, label, radius: _radius, valueLabel, ...progressProps} = props;

  // Apply autoColor if applicable
  const progressAmount = (props.value ?? 0) / (props.maxValue ?? 1);
  const color = !autoColor ? props.color :
    progressAmount < 0.5 ? (props.color ?? "primary") : progressAmount < 0.75 ? "warning" : "danger";

  const progress = (
    <ProgressBar color={color}
              {...progressProps}
              className={`${props.className ?? ""} DriveModeProgress`}>
      <ProgressBar.Track>
        <ProgressBar.Fill />
      </ProgressBar.Track>
    </ProgressBar>
  )

  // Overlay the props.valueLabel on the ProgressBar (if any)
  const valueLabelledProgress = valueLabel === undefined && children === undefined ? progress : (
    <div className="relative font-semibold">
      {progress}
      <div className="DriveModeProgressInnerText">
        {valueLabel}
        {children}
      </div>
      <div className="DriveModeProgressInnerTextDoubleUp">
        {valueLabel}
        {children}
      </div>
    </div>
  )

  // Prepend the props.label to the progress bar (if any)
  return label === undefined ? valueLabelledProgress : (
    <div className="flex flex-row items-center p-0 m-0">
      <div className="m-0 p-0">
        {label}
      </div>
      <div className="grow ml-2 m-0 p-0">
        {valueLabelledProgress}
      </div>
    </div>
  )
}
