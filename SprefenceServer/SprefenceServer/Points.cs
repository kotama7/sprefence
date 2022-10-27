using System;

namespace SprefenceServer
{
    [Serializable]
    public class Points
    {
        public Point[]? points { get; set; }
    }

    [Serializable]
    public class Point
    {
        public float x { get; set; }
        public float y { get; set; }
    }

    [Serializable]
    public class Command {
        public string? type { get; set; }
    }
}
