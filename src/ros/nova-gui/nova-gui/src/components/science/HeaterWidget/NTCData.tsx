import React from "react";
import {OverlayedProgress} from "../../OverlayedProgress/OverlayedProgress.tsx";
import {zip} from "lodash";

export interface NTCDataWidgetProps {
  tempReadings: number[]
  labels: string[]
  suffixes: string[]
}

const toTemp = (x: number) => 6 * x

const NTCData: React.FC<NTCDataWidgetProps> = ({tempReadings, labels, suffixes}) => {
  const displayList = zip(tempReadings, labels, suffixes)

  return (
    <div className="grid grid-cols-2 gap-4">
      {displayList.map(([reading, label, suffix]) => {
        if (reading !== undefined && label && suffix) {
          return (
            <div className="text-center">
              <OverlayedProgress key={label} size="lg" label={label} value={toTemp(reading)}>
                {toTemp(reading)} {suffix}
              </OverlayedProgress>
            </div>
          );
        }
      })
      }
    </div>
  )
}

export default NTCData