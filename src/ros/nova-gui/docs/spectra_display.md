## Using SpectraDisplay

Display an output graph alongside a scrollable window of graphs from a dataset!

Look on `feat/raman-spec` for a proper example of `< OutputComparison />` in action.

For more information on each file's uses, there is documentation within them:

- Styling is to be added in [`ChartOptions.ts`](../nova-gui/src/components/science/SpectraDisplay/ChartOptions.ts): [Apex Charts Docs](https://apexcharts.com/docs/react-charts/#)

- Peak finding (other than the default peak finder factory currently available) and any other chart analyses to be added in the future (maybe linear regression is cool/worth it in some contexts) is to be added in [`ChartAnalysis.ts`](../nova-gui/src/components/science/SpectraDisplay/ChartAnalysis.ts).

- Functionality within each chart should be modified in [`DataChart.tsx`](../nova-gui/src/components/science/SpectraDisplay/DataChart.tsx); e.g. adding better styling to peak annotations

- Functionality across the whole component/between charts should be modified in [`OutputComparison.tsx`](../nova-gui/src/components/science/SpectraDisplay/OutputComparison.tsx); e.g. adding a nicely coded interactive overlay of dataset charts on the left 'focus' chart for better 'output comparison'.