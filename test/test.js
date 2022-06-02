const actions = [
    {
      name: 'No decimation (default)',
      handler(chart) {
        chart.options.plugins.decimation.enabled = false;
        chart.update();
      }
    },
    {
      name: 'min-max decimation',
      handler(chart) {
        chart.options.plugins.decimation.algorithm = 'min-max';
        chart.options.plugins.decimation.enabled = true;
        chart.update();
      },
    },
    {
      name: 'LTTB decimation (50 samples)',
      handler(chart) {
        chart.options.plugins.decimation.algorithm = 'lttb';
        chart.options.plugins.decimation.enabled = true;
        chart.options.plugins.decimation.samples = 50;
        chart.update();
      }
    },
    {
      name: 'LTTB decimation (500 samples)',
      handler(chart) {
        chart.options.plugins.decimation.algorithm = 'lttb';
        chart.options.plugins.decimation.enabled = true;
        chart.options.plugins.decimation.samples = 500;
        chart.update();
      }
    }
  ];