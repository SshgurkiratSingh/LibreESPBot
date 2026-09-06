from setuptools import find_packages, setup
import os
from glob import glob

package_name = 'libreesp_ros_bridge'

setup(
    name=package_name,
    version='1.0.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='ROS Developer',
    maintainer_email='user@example.com',
    description='ROS 2 UDP Bridge for LibreESPBot',
    license='MIT',
    tests_require=['pytest'],
    entry_points={
        'console_scripts': [
            'bridge_node = libreesp_ros_bridge.bridge_node:main'
        ],
    },
)
