import { Progress, ProgressProps } from "@nextui-org/react"
import "./OverlayedProgress.css";

// Properties for the DriveModeButton component.
export interface IDriveOverlayedProps extends ProgressProps {
    // Set to true when you want the color of the bar to change to red for values over 75%
    autoColor?: boolean
}

export const OverlayedProgress: React.FC<IDriveOverlayedProps> = (props: IDriveOverlayedProps) =>
{
  const {autoColor: autoColor, ...progressProps} = props;

  // Apply autoColor if applicable
  const progressAmount = (props.value ?? 0) / (props.maxValue ?? 1);
  const color = !autoColor ? props.color :
    progressAmount < 0.5 ? (props.color ?? "primary") : progressAmount < 0.75 ? "warning" : "danger";

  const progress = (
    <Progress color={color}
              {...progressProps}
              className={`${props.className} DriveModeProgress`}
              label={undefined}
              valueLabel={undefined}>
      <p>hello</p>
    </Progress>
  )

  // Overlay the props.valueLabel on the Progress (if any)
  const valueLabelledProgress = props.valueLabel === undefined && props.children === undefined ? progress : (
    <div className="relative font-semibold">
      {progress}
      <div className="DriveModeProgressInnerText">
        {props.valueLabel}
        {props.children}
      </div>
      <div className="DriveModeProgressInnerTextDoubleUp">
        {props.valueLabel}
        {props.children}
      </div>
    </div>
  )

  // Prepend the props.label to the progress bar (if any)
  return props.label === undefined ? valueLabelledProgress : (
    <div className="flex flex-row items-center p-0 m-0">
      <div className="m-0 p-0">
        {props.label}
      </div>
      <div className="grow ml-2 m-0 p-0">
        {valueLabelledProgress}
      </div>
    </div>
  )
}
