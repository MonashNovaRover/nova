/*
 * Functions for chart analysis
 * Author: Connor Macdougall
*/

export const defaultPeakFinder = (data: number[][]): number[][] | undefined => {

    let resolution = 2;
    let peaks: number[][] = []
    let previous: boolean = true

    for (let middleIndex = 0; middleIndex < data.length; middleIndex++) {
        for (let range = middleIndex - resolution; range <= middleIndex + resolution; range++) {
            if (!previous) {
                break
            }
            if (range >= 0 || range < data.length) {
                if (range == middleIndex) {
                    previous = true
                } else if (range > middleIndex) {
                    if (range == data.length - 1 || data[range - 1][1] > data[range][1]) {
                        previous = true
                    } else {
                        previous = false
                    }
                } else {
                    if (range == 0 || data[range][1] < data[range + 1][1]) {
                        previous = true
                    } else {
                        previous = false
                    }
                }
            }
        }
        if (previous) {
            peaks.push(data[middleIndex]);
        }
    }
    return peaks;
}