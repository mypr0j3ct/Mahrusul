let glucoseChartInstance = null;
let uricAcidChartInstance = null;
export function initializeCharts() {
    const commonChartConfig = {
        type: 'line',
        options: {
            responsive: true,
            maintainAspectRatio: false,
            scales: {
                y: {
                    beginAtZero: false, 
                },
                x: {
                    ticks: {
                        maxRotation: 70, 
                        minRotation: 70
                    }
                }
            },
            plugins: {
                legend: {
                    display: false
                }
            },
            elements: {
                point: {
                    radius: 4 
                },
                line: {
                    tension: 0.1 
                }
            }
        }
    };
    const glucoseChartElement = document.getElementById('glucoseChart');
    const uricAcidChartElement = document.getElementById('uricAcidChart');
    if (glucoseChartElement) {
        glucoseChartInstance = new Chart(glucoseChartElement, {
            ...commonChartConfig,
            data: {
                labels: [],
                datasets: [{
                    label: 'Glucose Level',
                    data: [], 
                    borderColor: 'rgb(75, 192, 192)', 
                    backgroundColor: 'rgba(75, 192, 192, 0.2)',
                    fill: true,
                }]
            }
        });
    }
    if (uricAcidChartElement) {
        uricAcidChartInstance = new Chart(uricAcidChartElement, {
            ...commonChartConfig,
            data: {
                labels: [], 
                datasets: [{
                    label: 'Uric Acid Level',
                    data: [], 
                    borderColor: 'rgb(255, 99, 132)', 
                    backgroundColor: 'rgba(255, 99, 132, 0.2)',
                    fill: true,
                }]
            }
        });
    }
}
export function updateChartData(chartId, newLabels, newData) {
    let chartInstance;
    if (chartId === 'glucoseChart') {
        chartInstance = glucoseChartInstance;
    } else if (chartId === 'uricAcidChart') {
        chartInstance = uricAcidChartInstance;
    }

    if (chartInstance) {
        chartInstance.data.labels = newLabels;
        chartInstance.data.datasets[0].data = newData;
        chartInstance.update();
    } else {
        console.warn(`Chart instance for ${chartId} not found.`);
    }
}
