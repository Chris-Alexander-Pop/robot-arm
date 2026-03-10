import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D

# A sandbox script to start playing with Denavit-Hartenberg (DH) Parameters.
# This will calculate the Forward Kinematics (FK) and plot the arm in 3D!

def dh_transformation_matrix(alpha, a, d, theta):
    """
    Computes the transformation matrix from frame {i-1} to frame {i}
    using standard DH parameters.
    """
    alpha_rad = np.radians(alpha)
    theta_rad = np.radians(theta)
    
    T = np.array([
        [np.cos(theta_rad), -np.sin(theta_rad)*np.cos(alpha_rad),  np.sin(theta_rad)*np.sin(alpha_rad), a*np.cos(theta_rad)],
        [np.sin(theta_rad),  np.cos(theta_rad)*np.cos(alpha_rad), -np.cos(theta_rad)*np.sin(alpha_rad), a*np.sin(theta_rad)],
        [0,                  np.sin(alpha_rad),                    np.cos(alpha_rad),                   d],
        [0,                  0,                                    0,                                   1]
    ])
    return T

if __name__ == "__main__":
    print("--- 6-DOF Robot Arm DH Sandbox & Visualizer ---")
    
    # Classic DH Parameters [alpha, a, d, theta]
    dh_table = [
        {'alpha': 90, 'a': 0, 'd': 150, 'theta': 0},
        {'alpha': 0, 'a': 200, 'd': 0, 'theta': 0},
        {'alpha': 90, 'a': 0, 'd': 0, 'theta': 0},
        {'alpha': -90, 'a': 0, 'd': 150, 'theta': 0},
        {'alpha': 90, 'a': 0, 'd': 0, 'theta': 0},
        {'alpha': 0, 'a': 0, 'd': 50, 'theta': 0}
    ]
    
    # Example joint angles to test (in degrees)
    joint_angles = [45, 30, -30, 0, 90, 0]
    
    T = np.eye(4)
    # Store origin (0,0,0) as the first point in our 3D plot
    points = [T[:3, 3]]
    
    for i in range(6):
        theta_current = dh_table[i]['theta'] + joint_angles[i]
        T_link = dh_transformation_matrix(
            dh_table[i]['alpha'],
            dh_table[i]['a'],
            dh_table[i]['d'],
            theta_current
        )
        T = np.dot(T, T_link)
        points.append(T[:3, 3])
        
    points = np.array(points)
    
    print("\nEnd Effector Position (X, Y, Z):")
    print(f"X: {T[0,3]:.2f} mm\nY: {T[1,3]:.2f} mm\nZ: {T[2,3]:.2f} mm")
    
    # --- 3D Plotting ---
    fig = plt.figure()
    ax = fig.add_subplot(111, projection='3d')
    ax.plot(points[:, 0], points[:, 1], points[:, 2], '-o', linewidth=5, markersize=10, color='royalblue')
    ax.plot([points[0,0]], [points[0,1]], [points[0,2]], 'ro', markersize=12) # Highlight the Base
    ax.plot([points[-1,0]], [points[-1,1]], [points[-1,2]], 'go', markersize=12) # Highlight the Tip
    
    ax.set_xlabel('X (mm)')
    ax.set_ylabel('Y (mm)')
    ax.set_zlabel('Z (mm)')
    ax.set_title('6-DOF Arm Kinematic Plot')
    
    # Set equal aspect ratio so the arm doesn't look squished
    max_range = np.array([points[:,0].max()-points[:,0].min(), points[:,1].max()-points[:,1].min(), points[:,2].max()-points[:,2].min()]).max() / 2.0
    mid_x = (points[:,0].max()+points[:,0].min()) * 0.5
    mid_y = (points[:,1].max()+points[:,1].min()) * 0.5
    mid_z = (points[:,2].max()+points[:,2].min()) * 0.5
    ax.set_xlim(mid_x - max_range, mid_x + max_range)
    ax.set_ylim(mid_y - max_range, mid_y + max_range)
    ax.set_zlim(max(0, mid_z - max_range), mid_z + max_range) # Floor at Z=0
    
    plt.show()
