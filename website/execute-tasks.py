# -*- coding: utf-8 -*-

from app.models import Task
from app.logger import log_task


def main():
    for task in Task.load_all():
        if task.active and task.is_due:
            print 'Executing task #{} (due:{})'.format(task.id, task.time)
            success, response = task.execute()
            log_task(task, success, response)


if __name__ == '__main__':
    main()
