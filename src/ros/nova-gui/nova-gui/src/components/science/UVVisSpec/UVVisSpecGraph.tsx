import UVVisSpecGLGraph from "./UVVisSpecGLGraph.tsx";
import React from "react";
import { max } from "lodash";
import GLState from "../../../hooks/webgl/gl/GLState.ts";

export interface UVVisSpecGraphProps {
  // The data points to plot
  luminance: number[],
  colStartPercent: number,
  colEndPercent: number,

  startWavelength: number,
  endWavelength: number,

  wavelengthLabelCount: number,
  percentageLabelCount: number,

  onMouseMove: (event: React.MouseEvent<HTMLCanvasElement, MouseEvent>) => void,
  gl: GLState,
}


const UVVisSpecGraph: React.FC<UVVisSpecGraphProps> = (props) => {
  const luminance = props.luminance;
  const maxLuminance = (max(luminance) ?? 100) / 4.4167295593

  const xLabels = Array.from({ length: props.wavelengthLabelCount }, (_, i) => (
    `${(props.startWavelength + (props.endWavelength - props.startWavelength) * (i / (props.wavelengthLabelCount - 1))).toFixed(0)}`
  ));

  // Put the labels into a JSX
  const xLabelsElement = (
    <div className="grid auto-cols-fr grid-flow-col">
      <div className="text-center text-nowrap transform-gpu translate-x-[-50%] text-small">
        {xLabels[0]}
      </div>
      {xLabels.slice(1).map((label, index) => (
        <div
          key={index}
          className={index < xLabels.length - 2 ? "text-nowrap text-center col-span-2 text-small"
            : "text-right text-small text-nowrap"}
        >
          {label}
        </div>
      ))}
    </div>
  )

  // Text to display units, etc for a label
  const xAxisHeading = (
    <div className="text-center">
      Wavelength (nm)
    </div>
  )

  const yLabels = Array.from({ length: props.percentageLabelCount }, (_, i) => (
    `${(maxLuminance * ((i + 1) / (props.percentageLabelCount))).toFixed(0)}%`
  )).reverse();

  // Put the labels into a JSX
  const yLabelsElement = (
    <div className="grid auto-rows-fr grid-flow-row grid-cols-1 content-around pr-2 w-12 text-nowrap">
      <div className="text-right h-fit translate-y-[-50%] text-small">
          {yLabels[0]}
      </div>
      {yLabels.slice(1).map((label, index) => (
        <div
          key={index}
          className={"flex flex-col row-span-2"}
        >
          <div className="flex-grow"></div>
          <div className="text-right text-small">
            {label}
          </div>
          <div className="flex-grow"></div>
        </div>
      ))}
      <div></div>
    </div>
  )

  // Put those together into a chart
  return (
    <div className="grid grid-cols-[auto_1fr] grid-rows-[1fr_auto]">
      {yLabelsElement}
      <UVVisSpecGLGraph luminance={luminance}
        wavelengthLineCount={props.wavelengthLabelCount}
        percentageLineCount={props.percentageLabelCount}
        onMouseMove={props.onMouseMove}
        startWavelength={props.startWavelength}
        endWavelength={props.endWavelength}
        gl={props.gl}
      >
      </UVVisSpecGLGraph>
      <div></div>
      {xLabelsElement}
      <div></div>
      {xAxisHeading}
    </div>
  );
}

export default UVVisSpecGraph;
