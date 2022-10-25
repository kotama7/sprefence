using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace SprefenceServer
{
    internal class PortContent
    {
        public ObservableCollection<string> Ports { get; set; }

        public PortContent()
        {
            Ports = new ObservableCollection<string>();
        }
    }
}
