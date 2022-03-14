import { useEffect, useState } from 'react';
import { AppSession } from '../interfaces';

import Chart from 'chart.js/auto';
import { ChartDataset, ChartOptions } from 'chart.js';

interface PieChartAppPacketsProps {
  appSessions: AppSession[];
  start: Date;
  end: Date;
  isDarkMode: boolean;
}

let pieChart: Chart;

function PieChartAppPackets(props: PieChartAppPacketsProps) {
  const [data, setData] = useState({
    labels: [] as string[],
    datasets: [] as ChartDataset[],
  });

  useEffect(() => {
    const tmpData = {
      labels: [] as string[],
      datasets: [] as ChartDataset[],
    };

    const dataset = {
      label: 'Application Data Usage',
      data: [] as number[],
      backgroundColor: [] as string[],
      hoverOffset: 4,
    };

    props.appSessions.forEach((as) => {
      tmpData.labels.push(as.application.name);
      dataset.backgroundColor?.push(as.application.colorHex);
      let sum = 0;
      as.sessions.forEach((s) => {
        sum += s.pktTx + s.pktRx;
      });
      dataset.data.push(sum);
    });

    tmpData.datasets.push(dataset);
    setData(tmpData);
  }, [props.appSessions, props.isDarkMode]);

  useEffect(() => {
    buildChart();
  }, [data]);

  const buildChart = () => {
    const options: ChartOptions = {
      animation: {
        duration: 0,
      },
      plugins: {
        title: {
          display: true,
          text: 'Packets Tx/Rx per Application',
          font: {
            weight: 'bold',
            size: 16,
          },
        },
      },
    };

    let canvas = document.getElementById('pc-app-packets') as HTMLCanvasElement;
    if (canvas && data.labels.length > 0 && pieChart) {
      pieChart.data = data;
      pieChart.options = options;
      pieChart.update();
      return;
    }

    const wrapper = document.getElementById('wrapper-pc-app-packets');
    if (wrapper === null) return;
    wrapper.innerHTML = `<canvas id="pc-app-packets" />`;

    canvas = document.getElementById('pc-app-packets') as HTMLCanvasElement;
    if (canvas === null) return;

    let ctx = canvas.getContext('2d');
    if (ctx === null) return;

    pieChart = new Chart(ctx, {
      type: 'pie',
      data: data,
      options: options,
    });
  };

  return (
    <div className="block-chart">
      <div id="wrapper-pc-app-packets"></div>
      <div className="chart-side-pane"></div>
    </div>
  );
}

export default PieChartAppPackets;
