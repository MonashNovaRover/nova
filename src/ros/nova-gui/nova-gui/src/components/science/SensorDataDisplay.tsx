import React, {useMemo} from "react";
import {OverlayedProgress} from "../OverlayedProgress/OverlayedProgress.tsx";
import {zip} from "lodash";

export interface NTCDataWidgetProps {
  values: number[]
  labels: string[]
  suffixes: string[]
}

/**
 * Component to display data in the form of overlayed progress bars.
 * @param values a list of values to display
 * @param labels labels of each value
 * @param suffixes suffixes to be displayed after each value (units eg "°C")
 * @constructor
 */
const SensorDataDisplay: React.FC<NTCDataWidgetProps> = ({values, labels, suffixes}) => {
  const displayList = useMemo(() => zip(values, labels, suffixes), [values, labels, suffixes])

  return (
    <div className="grid grid-cols-2 gap-4">
      {displayList.map(([value, label, suffix]) => {
        if (value !== undefined && label != undefined && suffix != undefined) {
          return (
            <div key={label} className="text-center">
              <OverlayedProgress key={`progress-${label}`} aria-label={label} size="lg" label={label} value={value}>
                {value} {suffix}
              </OverlayedProgress>
            </div>
          );
        }
      })
      }
    </div>
  )
}

export default SensorDataDisplay