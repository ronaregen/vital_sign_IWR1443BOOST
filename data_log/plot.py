import pandas as pd
import matplotlib.pyplot as plt

data = pd.read_csv('logData.csv')

sub_data = data.head(500)
plt.figure(figsize=(7, 5))
# plt.plot(data['outputFilterHeartOut'])
plt.plot(sub_data['outputFilterHeartOut'])
plt.show()
