import UVVisSpecGLGraph from "./UVVisSpecGLGraph.tsx";
import React from "react";

export interface UVVisSpecGraphProps {
  // The data points to plot
  luminance: number[],
  colStartPercent: number,
  colEndPercent: number,

  startWavelength: number,
  endWavelength: number,

  wavelengthLabelCount: number,
}


const UVVisSpecGraph: React.FC<UVVisSpecGraphProps> = (props) => {
  const luminance = props.luminance;

  const xLabels = Array.from({ length: props.wavelengthLabelCount }, (_, i) => (
    `${props.startWavelength + (props.endWavelength - props.startWavelength) * (i / (props.wavelengthLabelCount - 1))}`
  ));

  // Put the labels into a JSX
  const xLabelsElement = (
    <div className="grid auto-cols-fr grid-flow-col">
      <div className="text-center transform-gpu translate-x-[-50%] text-small">
        {xLabels[0]}
      </div>
      {xLabels.slice(1).map((label, index) => (
        <div
          key={index}
          className={index < xLabels.length - 2 ? "text-center col-span-2 text-small" : "text-right text-small"}
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

  const yLabels = Array.from({ length: 5 }, (_, i) => (
    `${(100 * (i / (props.wavelengthLabelCount - 1))).toFixed(0)}%`
  ));

  // Put the labels into a JSX
  const yLabelsElement = (
    <div className="grid auto-rows-fr grid-flow-row grid-cols-1 content-around pr-2 min-w-10">
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
  const chart = (
    <div className="grid grid-cols-[auto_1fr] grid-rows-[1fr_auto]">
      {yLabelsElement}
      <UVVisSpecGLGraph luminance={luminance}></UVVisSpecGLGraph>
      <div></div>
      {xLabelsElement}
      <div></div>
      {xAxisHeading}
    </div>
  )

  return chart;
}

export default UVVisSpecGraph;
