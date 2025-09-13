print(df.describe())  # Stats summary
print(df.isnull().sum())  # Missing values (should be 0)
import matplotlib.pyplot as plt
df.plot(kind='scatter', x='sepal length (cm)', y='sepal width (cm)', c='species', colormap='viridis')
plt.title('Iris Data Exploration')
plt.show()