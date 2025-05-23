import React, {useMemo} from "react";
import {OverlayedProgress} from "../../OverlayedProgress/OverlayedProgress.tsx";
import {zip} from "lodash";

export interface NTCDataWidgetProps {
  tempReadings: number[]
  labels: string[]
  suffixes: string[]
}

const toTemp = (x: number) => 6 * x

/**
 * Component to display data in the form of overlayed progress bars.
 * @param tempReadings a list of values to display
 * @param labels labels of each value
 * @param suffixes suffixes to be displayed after each value (units eg "°C")
 * @constructor
 */
const NTCData: React.FC<NTCDataWidgetProps> = ({tempReadings, labels, suffixes}) => {
  const displayList = useMemo(() => zip(tempReadings, labels, suffixes), [tempReadings, labels, suffixes])

  return (
    <div className="grid grid-cols-2 gap-4">
      {displayList.map(([reading, label, suffix]) => {
        if (reading !== undefined && label && suffix) {
          return (
            <div key={label} className="text-center">
              <OverlayedProgress key={`progress-${label}`} aria-label={label} size="lg" label={label} value={toTemp(reading)}>
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