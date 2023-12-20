import { Progress, ProgressProps, } from "@nextui-org/react"

// Properties for the DriveModeButton component.
export interface IDriveProgressProps extends ProgressProps {

}

// A button used for selecting a drive mode for the DriveWidget
export const DriveProgress: React.FC<IDriveProgressProps> = (props: IDriveProgressProps) =>
{
  const progress = (
    <Progress {...props}
              className={`${props.className} DriveModeProgress`}
              label={undefined}
              valueLabel={undefined}>
      <p>hello</p>

    </Progress>
  )

  // Overlay the props.valueLabel on the Progress (if any)
  const valueLabelledProgress = props.valueLabel === undefined && props.children === undefined ? progress : (
    <div className="relative">
      {progress}
      <div className="DriveModeProgressInnerText">
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
