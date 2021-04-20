
#ifndef TASK_H_
#define TASK_H_

class Task {
public:
    Task(){}
    virtual ~Task(){}

    virtual void run() = 0;
private:
};



#endif /* TASK_H_ */
