/*
 * Functions for chart analysis
 * Author: Connor Macdougall
*/

const defaultVariablePeakFinder = (resolution: number, peak_percentage_diff: number, data: number[][]): number[][] | undefined => {
    /*
    * peak_percentage_diff is best explained with an example: e.g. peak_percentage_diff=20 means peak must be 20% greater than values at either end of resolution range.
    * so for a peak with a value of 50 that is 'RESOLUTION' units to the right of it, and a value of 60 that is 'RESOLUTION' units to the left of it must be > 1.2*max(50,60) = 72)
    * The amount of points to each side of a peak that must show a downwards trend away from the peak
    */

    // eslint-disable-next-line prefer-const
    let peaks: number[][] = [];                                                                             
    const scale_factor = 1 + peak_percentage_diff/100;
    
    if (data.length > 0) {
        for (let middleIndex = 0; middleIndex < data.length; middleIndex++) {
            let previous: boolean = true
            for (let range = middleIndex - resolution; range <= middleIndex + resolution; range++) {
                if (!previous) {
                    break
                }
                if (range >= 0 && range < data.length) {
                    if (range == middleIndex) {
                        previous = true
                    } else if (range > middleIndex) {
                        if ((range == data.length - 1 && middleIndex + resolution > data.length - 1) || range == middleIndex + resolution) {    // Checking for scale factor
                            if (data[middleIndex][1] > scale_factor*data[range][1]) {
                                previous = true
                            } else {
                                previous = false
                            }
                        } else if (range == data.length - 1 || data[range - 1][1] > data[range][1]) {                                           // Checking downwards gradient away from peak
                            previous = true
                        } else {
                            previous = false
                        }
                    } else {
                        if ((range == 0 && middleIndex - resolution < 0) || range == middleIndex - resolution) {                                // Checking for scale factor
                            if (data[middleIndex][1] > scale_factor*data[range][1]) {
                                previous = true
                            } else {
                                previous = false
                            }
                        } else if (range == 0 || data[range][1] < data[range + 1][1]) {                                                         // Checking downwards gradient away from peak
                            previous = true
                        } else {
                            previous = false
                        }
                    }
                } else {
                    previous = true
                }
            }
            if (previous) {
                peaks.push(data[middleIndex]);
            }
        }
    }
    return peaks;
}

export const getDefaultPeakFinder = (resolution: number, peak_percentage_diff: number): (data: number[][]) => number[][] | undefined => {
    /*
    * Sets constants for peak finding function and returns that function.
    */
    return function defaultPeakFinder(data: number[][]) {
        return defaultVariablePeakFinder(resolution, peak_percentage_diff, data)
    }
}