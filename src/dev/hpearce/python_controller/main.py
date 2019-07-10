
import superball
import nengo_one_osc_no_readout
import numpy as np
import matplotlib.pyplot as plt
import nengo
import math

from datetime import datetime

def main():
    now = datetime.now()
    dt_string = now.strftime("%Y_%m_%d__%H_%M_%S")

    w_set = 0.9

    save_csv = False
    display_graph = True
    baseline_sine = True
    noisy_sine = True
    simulation_time = 62
    stabilise_time = 2
    exp_count = 0
    results = []
    gauss_std = 0.3

    robot = superball.SUPERBall(stabilise_time = stabilise_time)
    lif_model = nengo_one_osc_no_readout.get_model(robot, w_set, True, gauss_std = gauss_std)
    
    print('Robot created')

    experiments = [
        ("Nengo LIF ("+str(gauss_std)+")", lif_model),
        #("Nengo LIFRate", lifrate_model),
    ]

    # baseline_sine gives us an idea of our performance
    if baseline_sine:
        robot.reset()

        master_osc = [0] * 8        

        print("Sine simulation time: 0", end='')
        while robot.lifetime < simulation_time:
            print("\rSine simulation time: " + str(robot.lifetime), end='')
            sTime = (w_set/6 * robot.lifetime)
            osc = math.sin(2*math.pi*sTime)
                
            master_osc[0] = 1.1 * osc
            master_osc[1] = -1.1 * osc
            master_osc[2] = -1.1 * osc
            master_osc[3] = 1.1 * osc
            master_osc[4] = 1.1 * osc
            master_osc[5] = -1.1 * osc
            master_osc[6] = -1.1 * osc
            master_osc[7] = 1.1 * osc
            
            robot.__call__(0.001, master_osc)
        
        print("\nSine simulation done.")
        result = ('Pure Sinusoid', [pos[::2] for pos in robot.com_history])
        if save_csv:
            with open(dt_string + '_results'+str(exp_count)+'_w=' + str(result[0])+'.csv', 'w') as file:
                file.write('row,col\n')
                positions = result[1]
                for position in positions:
                    file.write(str(position[0]) + ","  + str(position[1]) + "\n")
        
        if display_graph:
            results.append( result, )

        exp_count += 1

    if noisy_sine:
        robot.reset()

        master_osc = [0] * 8   
        gauss_dist = nengo.dists.Gaussian(mean=0,std=gauss_std)     

        print("Noisy sine simulation time: 0", end='')
        while robot.lifetime < simulation_time:
            print("\rNoisy sine simulation time: " + str(robot.lifetime), end='')
            sTime = (w_set/6 * robot.lifetime)
            noise = gauss_dist.sample(1)[0]
            if noise > 1: 
                noise = 1
            if noise < -1:
                noise = -1
            osc = math.sin(2*math.pi*sTime + noise) #* (1 + noise)
                
            master_osc[0] = 1.1 * osc
            master_osc[1] = -1.1 * osc
            master_osc[2] = -1.1 * osc
            master_osc[3] = 1.1 * osc
            master_osc[4] = 1.1 * osc
            master_osc[5] = -1.1 * osc
            master_osc[6] = -1.1 * osc
            master_osc[7] = 1.1 * osc
            
            robot.__call__(0.001, master_osc)
        
        print("\nNoisy sine simulation done.")
        result = ('Noisy Sinusoid ('+ str(gauss_std) + ')', [pos[::2] for pos in robot.com_history])
        if save_csv:
            with open(dt_string + '_results'+str(exp_count)+'_w=' + str(result[0])+'.csv', 'w') as file:
                file.write('row,col\n')
                positions = result[1]
                for position in positions:
                    file.write(str(position[0]) + ","  + str(position[1]) + "\n")
        
        if display_graph:
            results.append( result, )

        exp_count += 1

    for experiment in experiments:
        model = experiment[1]
        with nengo.Simulator(model) as sim:
            robot.reset()
            sim.run(simulation_time)
            result = (experiment[0], [pos[::2] for pos in robot.com_history])

            if save_csv:
                with open(dt_string + '_results'+str(exp_count)+'_w=' + str(result[0])+'.csv', 'w') as file:
                    file.write('row,col\n')
                    positions = result[1]
                    for position in positions:
                        file.write(str(position[0]) + ","  + str(position[1]) + "\n")

            if display_graph:
                results.append(result, )

        exp_count += 1

    
    

    if display_graph:
        #color_i = 0
        #color_N = len(results)
        #cmap = plt.get_cmap('jet_r')
        fig = plt.figure()
        ax = fig.add_subplot(111)
        for result in results:
            #color = cmap(float(color_i)/color_N)
            label = "w = " + str(result[0])
            positions = result[1]
            ax.plot(*zip(*positions), label=label)
            #color_i += 1
        ax.legend()
        ax.set_xlabel('X Displacement (m)')
        ax.set_ylabel('Z Displacement (m)')
        plt.title('60s Movement Simulation')
        plt.show()

if __name__ == '__main__':
    main()


